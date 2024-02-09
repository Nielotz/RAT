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
    const uint32_t payloadSize = 0; // In bytes.
    const char *payload = nullptr;

    Packet() = default;

    Packet(const Packet &packet);

    Packet(uint32_t payloadSize, PacketType packetType, const char *payload);

    ~Packet();
};

struct UndefinedPacket : Packet {
    const PacketType packetType = PacketType::UNDEFINED;

    UndefinedPacket();

    UndefinedPacket(uint32_t payloadSize, const char *payload);
};

struct DebugPacket : Packet {
    const PacketType packetType = PacketType::DEBUG;
    std::string message = {};

    template<typename... T>
    explicit DebugPacket(T... args);

    explicit DebugPacket(std::string message);

    DebugPacket(uint32_t payloadSize, const char *payload);

    static DebugPacket *unpack(uint32_t payloadSize, const char *payload);

    static DebugPacket *unpack(const Packet *packet);

    Packet *pack() const;
};


struct HandshakePacket : Packet {
    const PacketType packetType = PacketType::HANDSHAKE;
    std::string tempMessage = "HANDSHAKE";

    HandshakePacket(uint32_t payloadSize, const char *payload);

    static HandshakePacket *unpack(const Packet *packet);

    static HandshakePacket *unpack(uint32_t payloadSize, const char *payload);

    Packet *pack() const;
};


#endif //PACKET_H
