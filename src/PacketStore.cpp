#include "PacketStore.hpp"

#include <mutex>

void PacketStore::addPacket(packetscope::ParsedPacket parsedPacket) {
    std::unique_lock lock(mutex_);
    parsedPacket.id = ++id_;
    parsedPackets_.push_back(std::move(parsedPacket));
}

packetscope::ParsedPacket PacketStore::getById(int id) const {
    std::shared_lock lock(mutex_);
    /**
     * Think about exception handling.
     * There is no possibility that the id selected from the QT GUI
     * will directly return 0 because the packets will start from 1
     * and a selection greater than the size of the vector cannot be made.
     * Perhaps I don't need to handle it at all.
     */
    return parsedPackets_.at(static_cast<std::size_t>(id - 1));
}

std::size_t PacketStore::count() const {
    std::shared_lock lock(mutex_);
    return parsedPackets_.size();
}

std::vector<packetscope::ParsedPacket> PacketStore::getAllPackets() const {
    std::shared_lock lock(mutex_);
    return parsedPackets_;
}

void PacketStore::clear() {
    std::unique_lock lock(mutex_);
    /**
     * Might be use clear() and shrink_to_fit() but it's 2 operation and
     * shrink_to_fit() is non-binding. It means not guaranteed!
     * clear() removes all elements so size becomes 0 but doesn't free
     * allocated memory (capacity).
     * If going to reuse the vector, just clearing it might be more efficient.
     * Need to think about it.
     */
    std::vector<packetscope::ParsedPacket>().swap(parsedPackets_);
    // Reset the IDs when cleared the vector.
    id_ = 0;
}