#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <string>
#include <vector>
#include <time.h>
#include <cstdint>

#include <RawPacket.h>

/**
 * @file Types.hpp
 * @brief Core data structures for packet capture pipeline.
 */

namespace packetscope {

/**
 * @brief Raw captured packet data.
 * Contains a deep copy of the captured packet bytes and metadata.
 * Produced by PacketCapture and consumed by PacketProcessor.
 */
struct RawPacketData {
    timespec timestamp;
    std::vector<uint8_t> rawData;
    int rawDataLen;
    int frameLength;
    pcpp::LinkLayerType linkLayerType;
};

/**
 * @brief Network capture device information.
 */
struct DeviceInfo {
    std::string name;
    std::string description;
};

/**
 * @brief Parsed packet ready for UI display and storage.
 */
struct ParsedPacket {
    int id{};                           ///< Unique packet ID
    timespec timestamp;                 ///< Capture timestamp
    int rawDataLen;                     ///< Raw data length
    int frameLength;                    ///< Original frame length
    std::vector<uint8_t> rawData;       ///< Raw bytes

    std::string srcAddr;                ///< Source address (IP or MAC)
    std::string dstAddr;                ///< Destination address (IP or MAC)
    std::string protocol;               ///< Highest layer protocol name
    std::string info;                   ///< Info column

    std::vector<std::string> layerSummaries;
};

}
#endif