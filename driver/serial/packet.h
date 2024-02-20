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
    AUTH,
    UNDEFINED,
};

/* Packet format:
 * [  uint32_t [[ uint8_t  ][       ]
 * [PayloadSize][PacketType][payload]
 */
struct Packet {
    const PacketType packetType = PacketType::UNDEFINED;
    std::vector<char> payload;

    Packet() = default;

    virtual ~Packet() = default;

    Packet(PacketType packetType, const std::vector<char> &payload);

    Packet(PacketType packetType, const std::string &payload);

    virtual std::string str() const;

    virtual std::unique_ptr<Packet> pack() const;
};

struct UndefinedPacket : Packet {
    const PacketType packetType = PacketType::UNDEFINED;

    explicit UndefinedPacket(const std::vector<char> &payload);

    ~UndefinedPacket() override = default;

    std::unique_ptr<Packet> pack() const override = 0;
};

struct DebugPacket final : Packet {
    const PacketType packetType = PacketType::DEBUG;
    std::string message = {};

    // #ifdef __cpp_fold_expressions
    template<typename... T>
    explicit DebugPacket(T... args);

    // #endif

    explicit DebugPacket(std::string message);

    explicit DebugPacket(const char *message);

    ~DebugPacket() override = default;

    static std::unique_ptr<DebugPacket> unpack(const std::vector<char> &payload);

    static std::unique_ptr<DebugPacket> unpack(const Packet &packet);

    static std::unique_ptr<DebugPacket> unpack(const std::unique_ptr<Packet> &packet);

    std::unique_ptr<Packet> pack() const override;
};

/*
 * payload (only SYN and SYN_ACK sends ACK number):
 * [   uint8_t    ]{ uint32_t }
 * [HandshakeStage]{ACK number}
 */
struct HandshakePacket final : Packet {
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

    static std::unique_ptr<HandshakePacket> unpack(const std::vector<char> &payload);

    std::unique_ptr<Packet> pack() const override;

    std::string str() const override;
};

/*
 * [uint8_t ][  uint8_t  ][string ]
 * [AuthType][payloadSize][payload]
 */
struct AuthPacket final : Packet {
    enum class AuthType : uint8_t {
        SET_USER, SET_USER_RESPONSE,
        CHECK_USER, CHECK_USER_RESPONSE,
        REVOKE_USER, REVOKE_USER_RESPONSE,
        INVALID,
    };

    const PacketType packetType = PacketType::AUTH;
    const AuthType authType = AuthType::INVALID;
    std::string payload;

    static std::unique_ptr<AuthPacket> unpack(const std::unique_ptr<Packet> &packet);

    std::unique_ptr<Packet> pack() const override;

    explicit AuthPacket(AuthType authType, const std::string &payload);
    ~AuthPacket() override = default;
};


#endif //PACKET_H
