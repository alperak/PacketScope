#ifndef PACKETSTORE_HPP_
#define PACKETSTORE_HPP_

#include "Types.hpp"

#include <vector>
#include <shared_mutex>

/**
 * @brief Thread safe storage for parsed network packets.
 *
 * PacketStore provides a thread safe container for storing and
 * retrieving parsed packets. It uses a std::shared_mutex to allow
 * multiple concurrent readers while ensuring exclusive access
 * for writers.
 *
 * Thread Safety:
 *   - Multiple threads can read simultaneously (shared lock)
 *   - Only one thread can write at a time (exclusive lock)
 */
class PacketStore {
public:
    PacketStore() = default;
    ~PacketStore() = default;

    PacketStore(const PacketStore&) = delete;
    PacketStore& operator=(const PacketStore&) = delete;
    PacketStore(PacketStore&&) = delete;
    PacketStore& operator=(PacketStore&&) = delete;

    /**
     * @brief Adds a parsed packet to the store.
     *
     * Assigns a unique sequential ID to the packet before storing.
     * This method acquires an exclusive lock.
     *
     * @param parsedPacket The packet to store (moved into storage)
     */
    void addPacket(packetscope::ParsedPacket parsedPacket);

    /**
     * @brief Retrieves a packet by its ID.
     *
     * @param id The packet ID
     * @return The packet with the specified ID
     */
    packetscope::ParsedPacket getById(int id) const;

    /**
     * @brief Returns the number of stored packets.
     *
     * @return Current packet count
     */
    std::size_t count() const;

    /**
     * @brief Returns all stored packets.
     *
     * @return Vector with containing all packets
     */
    std::vector<packetscope::ParsedPacket> getAllPackets() const;

    /**
     * @brief Removes all packets and resets ID counter.
     *
     * Releases all memory used by stored packets.
     */
    void clear();

private:
    std::vector<packetscope::ParsedPacket> parsedPackets_;
    mutable std::shared_mutex mutex_;
    int id_{};
};

#endif