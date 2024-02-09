#ifndef PACKET_H
#define PACKET_H
#include <cstdint>
#include <string>

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
 * [ uint8_t  ][uint32_t[[    ]
 * [PacketType][DataSize][data]
 */
struct Packet {
    const PacketType packetType = PacketType::UNDEFINED;
    uint32_t payloadSize = 0; // In bytes.

    char *payload = nullptr;

    explicit Packet(const Packet *packet);

    Packet(uint32_t payloadSize, PacketType packetType, const char *payload);

    Packet() = default;

    ~Packet();
};

struct UndefinedPacket : Packet {
    const PacketType packetType = PacketType::UNDEFINED;

    UndefinedPacket();

    UndefinedPacket(uint32_t payloadSize, char *payload);
};

struct DebugPacket : Packet {
    const PacketType packetType = PacketType::DEBUG;
    std::string message = {};

    explicit DebugPacket(std::string message);

    DebugPacket(uint32_t payloadSize, char *payload);

    static DebugPacket unpack(const Packet *packet);

    static DebugPacket unpack(uint32_t payloadSize, char *payload);

    Packet *pack();
};


struct HandshakePacket : Packet {
    const PacketType packetType = PacketType::HANDSHAKE;
    std::string tempMessage = "HANDSHAKE";

    HandshakePacket();

    HandshakePacket(uint32_t payloadSize, char *payload);

    static HandshakePacket unpack(const Packet *packet);

    static HandshakePacket unpack(uint32_t payloadSize, char *payload);

    Packet *pack();
};


#endif //PACKET_H
