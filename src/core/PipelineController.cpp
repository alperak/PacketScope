#include "core/PipelineController.hpp"

#include <spdlog/spdlog.h>

PipelineController::PipelineController()
    : packetStore_(std::make_shared<PacketStore>())
    , packetCapture_(std::make_unique<PacketCapture>())
    , threadPool_(std::make_unique<ThreadPool>(kWorkerCount)) {}

PipelineController::~PipelineController() {
    stop();
}

std::vector<packetscope::DeviceInfo> PipelineController::listAvailableDevices() const {
    return packetCapture_->listAvailableDevices();
}

bool PipelineController::start(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(controlMutex_);

    // Check if already running or still shutting down from previous stop
    if (isRunning_) {
        spdlog::warn("PipelineController::start() - Already running");
        return false;
    }

    // Recreate ThreadPool if it was shut down from presvious stop
    if (threadPool_->isStopped()) {
        spdlog::debug("PipelineController::start() - Recreating ThreadPool");
        threadPool_ = std::make_unique<ThreadPool>(kWorkerCount);
    }

    // Start PacketCapture
    bool isPacketCaptureSuccess = packetCapture_->start(deviceName, [this](packetscope::RawPacketData rawPacket) {
        rawPacketQueue_.push(std::move(rawPacket));
    });

    if (!isPacketCaptureSuccess) {
        spdlog::error("PipelineController::start() - Failed to start packet capture on '{}'", deviceName);
        return false;
    }

    // Store device name for restart
    currentDeviceName_ = deviceName;

    // Start dispatcher thread
    dispatcherThread_ = std::thread([this] {
        try {
            spdlog::debug("PipelineController::start() - Dispatcher thread started");
            while (true) {
                // std::optional for poison pill pattern (nullopt = shutdown)
                std::optional<packetscope::RawPacketData> rawPacket = rawPacketQueue_.pop();

                // Check for poison pill (shutdown signal)
                if (!rawPacket) {
                    spdlog::debug("PipelineController::start() - Dispatcher received poison pill, exiting");
                    break;
                }

                // Submit parsing task to thread pool
                threadPool_->submit([this, raw = std::move(*rawPacket)]() mutable {
                    packetscope::ParsedPacket parsedPacket = packetProcessor_.process(raw);
                    packetStore_->addPacket(std::move(parsedPacket));
                });
            }
            spdlog::debug("PipelineController::start() - Dispatcher thread exited");
        } catch (const std::exception& e) {
            // Catch standard exceptions
            spdlog::error("PipelineController::start() - Dispatcher thread exception: {}", e.what());
        } catch (...) {
            // Catch all other exceptions
            spdlog::error("PipelineController::start() - Dispatcher thread unknown exception");
        }
    });

    isRunning_ = true;
    spdlog::info("PipelineController::start() - Pipeline started successfully on '{}'", deviceName);
    return true;
}

void PipelineController::stop() {
    std::lock_guard<std::mutex> lock(controlMutex_);

    if (!isRunning_) {
        // Already stopped
        return;
    }

    spdlog::info("PipelineController::stop() - Stopping pipeline");

    // Stop packet capture (no new packets)
    packetCapture_->stop();

    // Send poison pill to dispatcher (std::nullopt)
    rawPacketQueue_.push(std::nullopt);

    // Wait for dispatcher to finish
    if (dispatcherThread_.joinable()) {
        dispatcherThread_.join();
        spdlog::debug("PipelineController::stop() - Dispatcher thread joined");
    }

    // Shutdown thread pool
    threadPool_->shutdown();

    isRunning_ = false;
    spdlog::info("PipelineController::stop() - Pipeline stopped");
}

bool PipelineController::restart() {
    std::lock_guard<std::mutex> lock(controlMutex_);

    // Check if we have a device to restart on
    if (currentDeviceName_.empty()) {
        spdlog::warn("PipelineController::restart() - No device set, call start() first");
        return false;
    }

    spdlog::info("PipelineController::restart() - Restarting pipeline on '{}'", currentDeviceName_);

    // Stop if currently running
    if (isRunning_) {
        packetCapture_->stop();
        rawPacketQueue_.push(std::nullopt);

        if (dispatcherThread_.joinable()) {
            dispatcherThread_.join();
        }

        threadPool_->shutdown();
        isRunning_ = false;
    }

    // Clear stored packets and reset counters
    packetStore_->clear();
    rawPacketQueue_.clear();

    // Recreate ThreadPool because previous one was shut down
    threadPool_ = std::make_unique<ThreadPool>(kWorkerCount);

    // Start fresh capture on same device
    bool isPacketCaptureSuccess = packetCapture_->start(currentDeviceName_, [this](packetscope::RawPacketData rawPacket) {
        rawPacketQueue_.push(std::move(rawPacket));
    });

    if (!isPacketCaptureSuccess) {
        spdlog::error("PipelineController::restart() - Failed to restart capture on '{}'", currentDeviceName_);
        return false;
    }

    // Start dispatcher thread
    dispatcherThread_ = std::thread([this] {
        try {
            spdlog::debug("PipelineController::restart() - Dispatcher thread started");
            while (true) {
                std::optional<packetscope::RawPacketData> rawPacket = rawPacketQueue_.pop();

                if (!rawPacket) {
                    break;
                }

                threadPool_->submit([this, raw = std::move(*rawPacket)]() mutable {
                    packetscope::ParsedPacket parsedPacket = packetProcessor_.process(raw);
                    packetStore_->addPacket(std::move(parsedPacket));
                });
            }
        } catch (const std::exception& e) {
            spdlog::error("PipelineController::restart() - Dispatcher thread exception: {}", e.what());
        } catch (...) {
            spdlog::error("PipelineController::restart() - Dispatcher thread unknown exception");
        }
    });

    isRunning_ = true;
    spdlog::info("PipelineController::restart() - Pipeline restarted successfully");
    return true;
}

bool PipelineController::isRunning() const {
    return isRunning_;
}

std::shared_ptr<PacketStore> PipelineController::getStore() const {
    return packetStore_;
}

std::size_t PipelineController::queueSize() const {
    return rawPacketQueue_.size();
}

std::size_t PipelineController::capturedCount() const {
    return packetCapture_->getCapturedPacketCount();
}
std::size_t PipelineController::processedCount() const {
    return packetStore_->count();
}