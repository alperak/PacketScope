#include "ui/PacketListModel.hpp"

#include <ctime>
#include <stdexcept>

PacketListModel::PacketListModel(std::shared_ptr<PacketStore> store, QObject* parent)
    : QAbstractTableModel(parent)
    , store_(std::move(store)) {}

int PacketListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(cachedRowCount_);
}

int PacketListModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(ColumnType::COUNT);
}

QVariant PacketListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const int packetId = index.row() + 1;

    try {
        const auto packet = store_->getById(packetId);

        switch (static_cast<ColumnType>(index.column())) {
            case ColumnType::Id:          return packet.id;
            case ColumnType::Time:        return formatTimestamp(packet.timestamp);
            case ColumnType::Source:      return QString::fromStdString(packet.srcAddr);
            case ColumnType::Destination: return QString::fromStdString(packet.dstAddr);
            case ColumnType::Protocol:    return QString::fromStdString(packet.protocol);
            case ColumnType::Length:      return packet.frameLength;
            default:                      return QVariant();
        }

    } catch (const std::out_of_range&) {
        // Packet not found
        // Return empty variant view will display nothing
        return QVariant();
    } catch (const std::exception&) {
        // Other exceptions from PacketStore
        // Return empty variant to avoid crashing the UI
        return QVariant();
    }
}

QVariant PacketListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (static_cast<ColumnType>(section)) {
        case ColumnType::Id:          return QStringLiteral("No.");
        case ColumnType::Time:        return QStringLiteral("Time");
        case ColumnType::Source:      return QStringLiteral("Source");
        case ColumnType::Destination: return QStringLiteral("Destination");
        case ColumnType::Protocol:    return QStringLiteral("Protocol");
        case ColumnType::Length:      return QStringLiteral("Length");
        case ColumnType::COUNT:       // COUNT is not a real column
        default: return QVariant();
    }
}

int PacketListModel::getPacketId(int row) const {
    // Rows are 0 based, packet IDs are 1 based
    return row + 1;
}

void PacketListModel::refresh() {
    const std::size_t newCount = store_->count();

    if (newCount > cachedRowCount_) {
        beginInsertRows(
            QModelIndex(),
            static_cast<int>(cachedRowCount_),
            static_cast<int>(newCount - 1)
        );

        cachedRowCount_ = newCount;
        endInsertRows();
    }
}

void PacketListModel::reset() {
    beginResetModel();
    cachedRowCount_ = 0;
    endResetModel();
}

QString PacketListModel::formatTimestamp(const timespec& ts) const {
    std::tm localTime{};
    localtime_r(&ts.tv_sec, &localTime);

    return QTime(localTime.tm_hour, localTime.tm_min,
                 localTime.tm_sec).toString("HH:mm:ss");
}
