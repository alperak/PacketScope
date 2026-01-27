#include "core/PacketCapture.hpp"

#include <spdlog/spdlog.h>

PacketCapture::~PacketCapture() {
    stop();
}

std::vector<packetscope::DeviceInfo> PacketCapture::listAvailableDevices() const {
    const auto& devicesList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

    std::vector<packetscope::DeviceInfo> devices;
    devices.reserve(devicesList.size());

    for (const auto* device : devicesList)
        devices.emplace_back(packetscope::DeviceInfo{device->getName(), device->getDesc()});

    return devices;
}

bool PacketCapture::start(const std::string& deviceName, CaptureCallback callback) {
    if (!callback) {
        spdlog::error("PacketCapture::start() - Callback is empty");
        return false;
    }
    // If already running, don't start
    if (isRunning_) {
        spdlog::warn("PacketCapture::start() - Already running");
        return false;
    }

    device_ = pcpp::PcapLiveDeviceList::getInstance().getDeviceByName(deviceName);

    if (!device_) {
        spdlog::error("PacketCapture::start() - Device '{}' doesn't exist", deviceName);
        return false;
    }

    if (!device_->open()) {
        spdlog::error("PacketCapture::start() - Device '{}' cannot be opened", deviceName);
        return false;
    }

    callback_ = std::move(callback);

    /**
     * relevant log error is printed in any case:
     *  - Capture is already running
     *  - Device is not opened
     *  - Capture thread could not be created
     */
    if (!device_->startCapture(onPacketArrives, this)) {
        spdlog::error("PacketCapture::start() - Capture failed to start on '{}'", deviceName);
        device_->close();
        return false;
    }

    isRunning_ = true;
    spdlog::info("PacketCapture::start() - Packet capture started successfully on '{}'", deviceName);
    return true;
}

void PacketCapture::stop() {
    if (!isRunning_) {
        // Already stopped
        return;
    }

    spdlog::debug("PacketCapture::stop() - Stopping packet capture");

    if (device_) {
        device_->stopCapture();
        device_->close();
        device_ = nullptr;
    }

    isRunning_ = false;
    spdlog::debug("PacketCapture::stop() - Packet capture stopped");
}

void PacketCapture::onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) {
    PacketCapture* self = static_cast<PacketCapture*>(cookie);

    if (!self->callback_) {
        return;
    }

    self->capturedPacketCount_++;

    packetscope::RawPacketData rawPacketData;

    rawPacketData.timestamp = packet->getPacketTimeStamp();
    rawPacketData.frameLength = packet->getFrameLength();
    rawPacketData.rawDataLen = packet->getRawDataLen();
    rawPacketData.linkLayerType = packet->getLinkLayerType();

    const uint8_t* rawPtr = packet->getRawData();
    rawPacketData.rawData.assign(rawPtr, rawPtr + rawPacketData.rawDataLen);

    self->callback_(std::move(rawPacketData));
}

bool PacketCapture::isRunning() const {
    return isRunning_;
}

std::size_t PacketCapture::getCapturedPacketCount() const {
    return capturedPacketCount_;
}