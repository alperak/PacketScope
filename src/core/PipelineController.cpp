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

    // Start PacketCapture
    bool isPacketCaptureSuccess = packetCapture_->start(deviceName, [this](packetscope::RawPacketData rawPacket) {
        rawPacketQueue_.push(std::move(rawPacket));
    });

    if (!isPacketCaptureSuccess) {
        spdlog::error("PipelineController::start() - Failed to start packet capture on '{}'", deviceName);
        return false;
    }

    // Start dispatcher because PacketCapture started successfully
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