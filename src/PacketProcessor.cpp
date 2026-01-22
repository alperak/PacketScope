#include "PacketProcessor.hpp"

#include <Packet.h>
#include <EthLayer.h>
#include <IPv4Layer.h>
#include <IPv6Layer.h>

packetscope::ParsedPacket PacketProcessor::process(const packetscope::RawPacketData& rawPacketData) const {
    packetscope::ParsedPacket result{};

    // Copy metadata for hex view and packet list
    result.timestamp = rawPacketData.timestamp;
    result.rawData = rawPacketData.rawData;
    result.rawDataLen = rawPacketData.rawDataLen;
    result.frameLength = rawPacketData.frameLength;

    // Reconstruct PcapPlusPlus RawPacket from our copied data
    // We pass kDoNotDeleteRawData=false because we own the data in our vector.
    // PcapPlusPlus should not free it when RawPacket is destroyed.
    pcpp::RawPacket rawPacket(
        rawPacketData.rawData.data(),
        rawPacketData.rawDataLen,
        rawPacketData.timestamp,
        kDoNotDeleteRawData,
        rawPacketData.linkLayerType
    );

    // Parse raw packet
    pcpp::Packet parsedPacket(&rawPacket);
    // Get the lowest layer
    pcpp::Layer* layer = parsedPacket.getFirstLayer();

    // Walk through all layers
    while (layer) {
        // Store each layer summary for detail view
        result.layerSummaries.push_back(layer->toString());

        // Update protocol only if it's not a generic payload.
        // GenericPayload means "unrecognized data after the last known protocol".
        // We want to display the last recognized protocol, not "Unknown".
        if (layer->getProtocol() != pcpp::GenericPayload && layer->getProtocol() != pcpp::UnknownProtocol
            && layer->getProtocol() != pcpp::PacketTrailer) {
            // Protocol is updated for each layer. The last (highest) layer's 
            // protocol becomes the displayed protocol.
            // This matches Wireshark's behavior:
            // https://osqa-ask.wireshark.org/questions/21257/how-does-wireshark-determine-the-protocol/
            result.protocol = protocolTypeToString(layer->getProtocol());
        }

        switch (layer->getProtocol()) {
            case pcpp::Ethernet: {
                const pcpp::EthLayer* eth = static_cast<pcpp::EthLayer*>(layer);
                result.srcAddr = eth->getSourceMac().toString();
                result.dstAddr = eth->getDestMac().toString();
                break;
            }
            case pcpp::IPv4: {
                const pcpp::IPv4Layer* ip = static_cast<pcpp::IPv4Layer*>(layer);
                result.srcAddr = ip->getSrcIPAddress().toString();
                result.dstAddr = ip->getDstIPAddress().toString();
                break;
            }
            case pcpp::IPv6: {
                const pcpp::IPv6Layer* ip = static_cast<pcpp::IPv6Layer*>(layer);
                result.srcAddr = ip->getSrcIPAddress().toString();
                result.dstAddr = ip->getDstIPAddress().toString();
                break;
            }
            default:
                break;
        }
        // Move to next layer
        layer = layer->getNextLayer();
    }
    return result;
}

std::string PacketProcessor::protocolTypeToString(pcpp::ProtocolType protocolType) {
    switch (protocolType) {
        case pcpp::Ethernet:        return "Ethernet";
        case pcpp::ARP:             return "ARP";
        case pcpp::IPv4:            return "IPv4";
        case pcpp::IPv6:            return "IPv6";
        case pcpp::ICMP:            return "ICMP";
        case pcpp::ICMPv6:          return "ICMPv6";
        case pcpp::TCP:             return "TCP";
        case pcpp::UDP:             return "UDP";
        case pcpp::DNS:             return "DNS";
        case pcpp::HTTPRequest:     return "HTTP";
        case pcpp::HTTPResponse:    return "HTTP";
        case pcpp::SSL:             return "TLS";
        case pcpp::SSH:             return "SSH";
        case pcpp::FTP:             return "FTP";
        default:                    return "Unknown";
    }
}