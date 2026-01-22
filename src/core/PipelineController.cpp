#include "core/PipelineController.hpp"

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
    // If it's already running, don't start it
    if (isRunning_) {
        return false;
    }

    // Start PacketCapture
    bool isPacketCaptureSuccess = packetCapture_->start(deviceName, [this](packetscope::RawPacketData rawPacket) {
        rawPacketQueue_.push(std::move(rawPacket));
    });

    if (!isPacketCaptureSuccess)
        return false;

    // Start dispatcher because PacketCapture started successfully
    dispatcherThread_ = std::thread([this] {
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
    });

    isRunning_ = true;
    return true;
}

void PipelineController::stop() {
    if (!isRunning_) {
        return;
    }

    isRunning_ = false;

    packetCapture_->stop();

    // Sending Poison Pill for stop sign
    rawPacketQueue_.push(std::nullopt);

    if (dispatcherThread_.joinable()) {
        dispatcherThread_.join();
    }

    threadPool_->shutdown();
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