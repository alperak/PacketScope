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

/**
 * @brief Raw captured packet data.
 * Contains a deep copy of the captured packet bytes and metadata.
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
#endif