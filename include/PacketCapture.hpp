#ifndef PACKETCAPTURE_HPP_
#define PACKETCAPTURE_HPP_

#include <PcapLiveDeviceList.h>
#include <atomic>
#include <vector>

#include "Types.hpp"

/**
 * @brief Live packet capture helper based on PcapPlusPlus.
 *
 * PacketCapture provides a simple interface to:
 * - List available capture devices
 * - Start capturing packets on a selected device
 * - Receive raw packets via callback
 */
class PacketCapture {
public:
    using CaptureCallback = std::function<void(packetscope::RawPacketData)>;

    PacketCapture() = default;
    ~PacketCapture();
    PacketCapture(const PacketCapture&) = delete;
    PacketCapture& operator=(const PacketCapture&) = delete;
    PacketCapture(PacketCapture&&) = delete;
    PacketCapture& operator=(PacketCapture&&) = delete;

    /**
     * @brief List available live capture devices.
     * @return A vector of DeviceInfo containing device name and description.
     */
    std::vector<packetscope::DeviceInfo> listAvailableDevices() const;

    /**
     * @brief Starts asynchronous packet capture on the given network interface.
     *
     * Packets are captured asynchronously and delivered to the provided
     * callback function.
     *
     * @note Packet capture runs in a background thread managed by PcapPlusPlus.
     *       The callback is invoked from that background thread.
     *
     * @warning The callback must be thread safe.
     *
     * @param deviceName Name of the network interface to capture from
     * @param callback   Function invoked for each captured packet
     * @return true if capture started successfully, false otherwise
     */
    bool start(const std::string& deviceName, CaptureCallback callback);

    /**
     * @brief Stop the current packet capture session.
     * Safe to call multiple times.
     */
    void stop();

    /**
     * @brief Check whether packet capturing is currently active.
     * @return true if capture is running, false otherwise.
     */
    bool isRunning() const;

private:
    /**
     * @brief Internal callback invoked by PcapPlusPlus for each captured packet.
     */
    static void onPacketArrives(pcpp::RawPacket* packet,
                                pcpp::PcapLiveDevice* dev,
                                void* cookie);
    pcpp::PcapLiveDevice* device_{nullptr};
    std::atomic<bool> isRunning_{false};
    CaptureCallback callback_;
};

#endif