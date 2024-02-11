#ifndef PACKET_H
#define PACKET_H
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/*
 * Implemented packet types.
 *
 * PacketType with value of UNDEFINED or higher should be treated as UNDEFINED.
 */
enum class PacketType : uint8_t {
    DEBUG,
    HANDSHAKE,
    UNDEFINED,
};

/* Packet format:
 * [ uint8_t  ][  uint32_t [[       ]
 * [PacketType][PayloadSize][payload]
 */
struct Packet {
    const PacketType packetType = PacketType::UNDEFINED;
    std::vector<char> payload;

    Packet() = default;

    Packet(PacketType packetType, const std::vector<char> &payload);

    Packet(PacketType packetType, const std::string &payload);

    std::string str() const;
};

struct UndefinedPacket : Packet {
    const PacketType packetType = PacketType::UNDEFINED;

    explicit UndefinedPacket(const std::vector<char> &payload);
};

struct DebugPacket : Packet {
    const PacketType packetType = PacketType::DEBUG;
    std::string message = {};

// #ifdef __cpp_fold_expressions
    template<typename... T>
    explicit DebugPacket(T... args);
// #endif

    explicit DebugPacket(std::string message);
    explicit DebugPacket(const char* message);

    static std::unique_ptr<DebugPacket> unpack(const std::vector<char> &payload);

    static std::unique_ptr<DebugPacket> unpack(const Packet &packet);

    static std::unique_ptr<DebugPacket> unpack(const std::unique_ptr<Packet> &packet);

    std::unique_ptr<Packet> pack() const;
};

/*
 * payload (only SYN and SYN_ACK sends ACK number):
 * [   uint8_t    ]{ uint32_t }
 * [HandshakeStage]{ACK number}
 */
struct HandshakePacket : Packet {
    const PacketType packetType = PacketType::HANDSHAKE;

    enum class HandshakeStage : uint8_t {
        INVALID,
        SYN,
        ACK,
        SYN_ACK,
    };

    HandshakeStage stage = HandshakeStage::INVALID;
    uint32_t ackNumber = -1;

    explicit HandshakePacket(HandshakeStage stage, uint32_t ackNumber = -1);

    static std::unique_ptr<HandshakePacket> unpack(const std::unique_ptr<Packet> &packet);

    static std::unique_ptr<HandshakePacket> unpack(const std::vector<char>& payload);

    std::unique_ptr<Packet> pack() const;

    std::string str() const;
};


#endif //PACKET_H
