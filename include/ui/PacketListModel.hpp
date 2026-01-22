#ifndef PACKETLISTMODEL_HPP
#define PACKETLISTMODEL_HPP

#include "core/PacketStore.hpp"

#include <QAbstractTableModel>
#include <QTime>
#include <memory>

/**
 * @brief Qt Model for displaying captured network packets in a table view
 */
class PacketListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @enum ColumnType
     * @brief Defines the columns displayed in the packet table
     */
    enum class ColumnType {
        Id = 0,          ///< Packet sequence number (1 based)
        Time,            ///< Capture timestamp (HH:MM:SS)
        Source,          ///< Source IP address or MAC address
        Destination,     ///< Destination IP address or MAC address
        Protocol,        ///< Protocol name
        Length,          ///< Frame length in bytes
        COUNT            ///< Total number of columns (not a real column)
    };

    /**
     * @brief Constructs a PacketListModel with the given packet store
     *
     * @param store Shared pointer to the PacketStore containing captured packets
     * @param parent Optional parent QObject
     */
    explicit PacketListModel(std::shared_ptr<PacketStore> store, QObject* parent = nullptr);

    ~PacketListModel() override = default;

    PacketListModel(const PacketListModel&) = delete;
    PacketListModel& operator=(const PacketListModel&) = delete;
    PacketListModel(PacketListModel&&) = delete;
    PacketListModel& operator=(PacketListModel&&) = delete;

    /**
     * @brief Returns the number of rows(packets) in the model
     * @note Returns cached count for performance. Call refresh() to update cache.
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Returns the number of columns in the model
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Returns data for the given index and role
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Returns header data for the given section
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Converts a row number to its corresponding packet ID
     */
    int getPacketId(int row) const;

public slots:
    /**
     * @brief Updates the model with new packets from the store
     */
    void refresh();

    /**
     * @brief Clears all data and resets the model
     */
    void reset();

private:
    QString formatTimestamp(const timespec& ts) const;

    std::shared_ptr<PacketStore> store_;

    /// Cached row count to avoid repeated PacketStore::count() calls
    std::size_t cachedRowCount_{};
};

#endif
