#include "packet.h"

#include <sstream>
#include <utility>

#include "converters.h"

Packet::Packet(const PacketType packetType, const std::vector<char> &payload)
    : packetType(packetType), payload(payload) {
}

Packet::Packet(const PacketType packetType, const std::string &payload)
    : packetType(packetType), payload(payload.c_str(), payload.c_str() + payload.size() + 1) {
}


UndefinedPacket::UndefinedPacket(const std::vector<char> &payload)
    : Packet(PacketType::UNDEFINED, payload) {
}


template<typename... T>
DebugPacket::DebugPacket(T... args) {
    std::ostringstream mess;

    (mess << ... << args);

    this->message = mess.str();
}

DebugPacket::DebugPacket(std::string message)
    : message(std::move(message)) {
}

std::unique_ptr<DebugPacket> DebugPacket::unpack(const std::vector<char> &payload) {
    return std::make_unique<DebugPacket>(std::string(payload.begin(), payload.end()));
}

std::unique_ptr<DebugPacket> DebugPacket::unpack(const Packet &packet) {
    return unpack(packet.payload);
}

std::unique_ptr<DebugPacket> DebugPacket::unpack(const std::unique_ptr<Packet> &packet) {
    return unpack(packet->payload);
}

std::unique_ptr<Packet> DebugPacket::pack() const {
    return std::make_unique<Packet>(PacketType::DEBUG, this->message);
}


HandshakePacket::HandshakePacket(const HandshakeStage stage, const uint32_t ackNumber)
    : stage(stage), ackNumber(ackNumber) {
}

std::unique_ptr<HandshakePacket> HandshakePacket::unpack(const std::vector<char> &payload) {
    const auto &payloadSize = payload.size();

    if (payloadSize != sizeof(uint8_t) && payloadSize != sizeof(uint8_t) + sizeof(uint32_t))
        return nullptr;

    uint8_t stage = payload[0];
    if (stage == 0 || stage > static_cast<uint8_t>(HandshakeStage::SYN_ACK))
        stage = static_cast<uint8_t>(HandshakeStage::INVALID);
    const auto &packetStage = static_cast<HandshakeStage>(stage);

    if (payloadSize > 1) {
        uint32_t ackNumber = convert4x8BitsTo32<char, uint32_t>(payload, 1);

        return std::make_unique<HandshakePacket>(packetStage, ackNumber);
    }
    return std::make_unique<HandshakePacket>(packetStage);
}

std::unique_ptr<HandshakePacket> HandshakePacket::unpack(const std::unique_ptr<Packet> &packet) {
    return unpack(packet->payload);
}

std::unique_ptr<Packet> HandshakePacket::pack() const {
    std::vector<char> payload(5);

    switch (stage) {
        case HandshakeStage::SYN:
            payload[0] = static_cast<char>(this->stage);
            break;
        case HandshakeStage::ACK:
        case HandshakeStage::SYN_ACK:
            payload[0] = static_cast<char>(this->stage);
            convert32bitTo4<uint32_t, char>(ackNumber, &payload[1]);
            break;
        case HandshakeStage::INVALID:
        default:
            return nullptr;
    }
    return std::make_unique<Packet>(PacketType::HANDSHAKE, payload);
}
