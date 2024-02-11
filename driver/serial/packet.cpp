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

std::string Packet::str() const {
    std::stringstream repr;
    std::string packetTypeName;
    switch (packetType) {
        case PacketType::DEBUG:
            packetTypeName = "DEBUG";
            break;
        case PacketType::HANDSHAKE:
            packetTypeName = "HANDSHAKE";
            break;
        case PacketType::UNDEFINED:
            packetTypeName = "UNDEFINED";
            break;
        default: ;
    }

    repr << "Packet: PacketType: " << packetTypeName << ", payload(" << payload.size() << "): ";
    repr << "0x" << std::hex;
    for (const char &elem: payload)
        repr << static_cast<uint32_t>(elem);
    repr << std::dec;

    return repr.str();
}


UndefinedPacket::UndefinedPacket(const std::vector<char> &payload)
    : Packet(PacketType::UNDEFINED, payload) {
}

#ifdef __cpp_fold_expressions
template<typename... T>
DebugPacket::DebugPacket(T... args) {
    std::ostringstream mess;

    (mess << ... << args);

    this->message = mess.str();
}
#endif

DebugPacket::DebugPacket(std::string message)
    : message(std::move(message)) {
}

DebugPacket::DebugPacket(const char *message)
    : message(message) {
}

std::unique_ptr<DebugPacket> DebugPacket::unpack(const std::vector<char> &payload) {
    auto message = std::string(payload.begin(), payload.end());
    // if (message.ends_with('\0'))  // Requires C++20
    if (message[message.size() - 1] == '\0')
        message.pop_back();
    return std::make_unique<DebugPacket>(message);
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
    std::vector<char> payload;
    payload.reserve(5);
    payload.emplace_back(static_cast<char>(this->stage));

    switch (stage) {
        case HandshakeStage::SYN:
            break;
        case HandshakeStage::ACK:
        case HandshakeStage::SYN_ACK:
            payload.resize(5);
            convert32bitTo4<uint32_t, char>(ackNumber, payload, 1);
            break;
        case HandshakeStage::INVALID:
        default:
            return nullptr;
    }
    return std::make_unique<Packet>(PacketType::HANDSHAKE, payload);
}

std::string HandshakePacket::str() const {
    std::stringstream repr;
    std::string handshakeStage;
    switch (stage) {
        case HandshakeStage::INVALID:
            handshakeStage = "INVALID";
            break;
        case HandshakeStage::SYN:
            handshakeStage = "SYN";
            break;
        case HandshakeStage::ACK:
            handshakeStage = "ACK";
            break;
        case HandshakeStage::SYN_ACK:
            handshakeStage = "SYN_ACK";
            break;
        default: ;
    }
    repr << "HandshakeStage: " << handshakeStage << ", ackNumber: " << ackNumber;
    return repr.str();
}
