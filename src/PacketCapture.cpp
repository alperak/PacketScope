#include "PacketCapture.hpp"

#include <iostream>

PacketCapture::~PacketCapture() {
    stop();
}

std::vector<DeviceInfo> PacketCapture::listAvailableDevices() const {
    const auto& devicesList = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();

    std::vector<DeviceInfo> devices;
    devices.reserve(devicesList.size());

    for (const auto* device : devicesList)
        devices.emplace_back(DeviceInfo{device->getName(), device->getDesc()});

    return devices;
}

bool PacketCapture::start(const std::string& deviceName, CaptureCallback callback) {
    if (!callback) {
        std::cout << "PacketCapture::start() - Callback is empty.\n";
        return false;
    }
    // If already running, don't start
    if (isRunning_) {
        std::cout << "PacketCapture::start() - called while already running!\n";
        return false;
    }

    device_ = pcpp::PcapLiveDeviceList::getInstance().getDeviceByName(deviceName);

    if (!device_) {
        std::cout << "PacketCapture::start() - The device isn't exist.\n";
        return false;
    }

    if (!device_->open()) {
        std::cout << "PacketCapture::start() - The device couldn't be opened.\n";
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
        std::cout << "PacketCapture::start() - Capture couldn't be started.\n";
        device_->close();
        return false;
    }

    isRunning_ = true;
    return true;
}

void PacketCapture::stop() {
    if (!isRunning_)
        return;

    if (device_) {
        device_->stopCapture();
        device_->close();
    }

    isRunning_ = false;
}

void PacketCapture::onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie) {
    PacketCapture* self = static_cast<PacketCapture*>(cookie);

    if (!self->callback_) {
        return;
    }

    RawPacketData rawPacketData;

    rawPacketData.timestamp = packet->getPacketTimeStamp();
    rawPacketData.frameLength = packet->getFrameLength();
    rawPacketData.rawDataLen = packet->getRawDataLen();
    rawPacketData.linkLayerType = packet->getLinkLayerType();

    const uint8_t* rawPtr = packet->getRawData();
    rawPacketData.rawData.assign(rawPtr, rawPtr + rawPacketData.rawDataLen);

    self->callback_(std::move(rawPacketData));
}

bool PacketCapture::isRunning() const { return isRunning_; }