#ifndef PACKETPROCESSOR_HPP_
#define PACKETPROCESSOR_HPP_

#include "Types.hpp"

#include <ProtocolType.h>

/**
 * @brief Parses raw captured packets into structured form for UI display.
 *
 * Extracts protocol layers, addresses and metadata from packet.
 * Output is ready for view.
 *
 * @note This class is stateless and thread safe. Multiple threads can call
 * process() simultaneously on the same instance.
 */
class PacketProcessor {
public:
    /**
     * @brief Process a raw captured packet into a ParsedPacket.
     *
     * Walks through all layers from lowes to highest
     * - Source/destination addresses (IP overwrites MAC if present)
     * - Protocol name (highest recognized layer, excluding payload)
     * - Layer summaries for detail view (via PcapPlusPlus toString())
     *
     * @param rawPacketData Raw packet bytes and capture metadata from PacketCapture
     * @return ParsedPacket containing all extracted information, ready for UI display
     */
    packetscope::ParsedPacket process(const packetscope::RawPacketData& rawPacketData) const;
private:
    /**
     * @brief Convert PcapPlusPlus protocol enum to string.
     *
     * @param protocolType Protocol type from PcapPlusPlus
     * @return Display string
     */
    static std::string protocolTypeToString(pcpp::ProtocolType protocolType);

    /// Flag for pcpp::RawPacket constructor.
    constexpr static bool kDoNotDeleteRawData{false};
};

#endif