#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <cstdio>
#include <iostream>

#include "serial/serial.h"

constexpr auto path = "/dev/serial/by-id/usb-WEMOS.CC_LOLIN-S2-MINI_0-if00";

constexpr pam_conv conv = {
    misc_conv, // Defined in pam_misc.h
    nullptr
};

using std::cout;
using std::cerr;
using std::endl;
using std::string;

bool handshake(const Serial &serial) {
    if (!serial.writePacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN)))
        return false;

    while (true) {
        cout << "Waiting for a packet..." << endl;
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 100);
            packet = serial.readPacket();
        }

        switch (packet->packetType) {
            case PacketType::DEBUG: {
                const auto &debugPacket = DebugPacket::unpack(packet);
                cout << "Packet type is DEBUG. Message: " << debugPacket->message << endl;
            }
            break;
            case PacketType::HANDSHAKE:
                switch (const auto &handshakePacket = HandshakePacket::unpack(packet);
                    handshakePacket->stage) {
                    case HandshakePacket::HandshakeStage::SYN_ACK:
                        cout << "Packet is SYN_ACK HANDSHAKE. ACK number: " << handshakePacket->ackNumber << endl;
                        if (!serial.writePacket(
                            HandshakePacket(HandshakePacket::HandshakeStage::ACK,
                                            handshakePacket->ackNumber + 1)))
                            return false;
                        return true;
                    case HandshakePacket::HandshakeStage::SYN:
                        cout << "Packet is SYN HANDSHAKE. Not supported  on PC side." << endl;
                        return false;
                    case HandshakePacket::HandshakeStage::ACK:
                        cout << "Packet is ACK HANDSHAKE. Not supported  on PC side." << endl;
                        return false;
                    default:
                    case HandshakePacket::HandshakeStage::INVALID:
                        cout << "Packet is INVALID HANDSHAKE." << endl;
                        return false;
                }
                break;
            case PacketType::UNDEFINED:
                cout << "Packet type is undefined: " << static_cast<uint32_t>(packet->packetType) << endl;
                return false;
            default: ;
        }
    }
}

bool authSetUser(const Serial &serial, const string &username) {
    if (!serial.writePacket(AuthPacket(AuthPacket::AuthType::SET_USER, username)))
        return false;

    while (true) {
        cout << "Waiting for a packet..." << endl;
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 100);
            packet = serial.readPacket();
        }

        switch (packet->packetType) {
            case PacketType::DEBUG: {
                const auto &debugPacket = DebugPacket::unpack(packet);
                cout << "Packet type is DEBUG. Message: " << debugPacket->message << endl;
            }
            break;
            case PacketType::AUTH: {
                cout << "Packet is AUTH." << endl;
                const auto &authPacket = AuthPacket::unpack(packet);
                if (authPacket == nullptr) {
                    cerr << "Auth packet is nullptr." << endl;
                    return false;
                }
                switch (authPacket->authType) {
                    default: ;
                        return false;
                    case AuthPacket::AuthType::CHECK_USER:
                        cout << "Packet is AuthType::CHECK_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::CHECK_USER_RESPONSE:
                        cout << "Packet is AuthType::CHECK_USER_RESPONSE: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::SET_USER:
                        cout << "Packet is AuthType::SET_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::SET_USER_RESPONSE:
                        cout << "Packet is AuthType::SET_USER_RESPONSE: " << authPacket->payload << endl;
                        return authPacket->payload == "OK";
                }
                break;
            }
            case PacketType::HANDSHAKE:
                cout << "Packet is HANDSHAKE." << endl;
                return false;
            case PacketType::UNDEFINED:
                cout << "Packet type is undefined: " << static_cast<uint32_t>(packet->packetType) << endl;
                return false;
            default: ;
        }
    }
}

bool authCheckUser(const Serial &serial, const string &username) {
    if (!serial.writePacket(AuthPacket(AuthPacket::AuthType::CHECK_USER, username)))
        return false;

    while (true) {
        cout << "Waiting for a packet..." << endl;
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 100);
            packet = serial.readPacket();
        }

        switch (packet->packetType) {
            case PacketType::DEBUG: {
                const auto &debugPacket = DebugPacket::unpack(packet);
                cout << "Packet type is DEBUG. Message: " << debugPacket->message << endl;
            }
            break;
            case PacketType::AUTH: {
                cout << "Packet is AUTH." << endl;
                const auto &authPacket = AuthPacket::unpack(packet);
                if (authPacket == nullptr) {
                    cerr << "Auth packet is nullptr." << endl;
                    return false;
                }
                switch (authPacket->authType) {
                    default: ;
                        return false;
                    case AuthPacket::AuthType::CHECK_USER:
                        cout << "Packet is AuthType::CHECK_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::CHECK_USER_RESPONSE:
                        cout << "Packet is AuthType::CHECK_USER_RESPONSE: " << authPacket->payload << endl;
                        return authPacket->payload == "VERIFIED";
                    case AuthPacket::AuthType::SET_USER:
                        cout << "Packet is AuthType::SET_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::SET_USER_RESPONSE:
                        cout << "Packet is AuthType::SET_USER_RESPONSE: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::REVOKE_USER:
                        cout << "Packet is AuthType::SET_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::REVOKE_USER_RESPONSE:
                        cout << "Packet is AuthType::SET_USER_RESPONSE: " << authPacket->payload << endl;
                        break;
                }
                break;
            }
            case PacketType::HANDSHAKE:
                cout << "Packet is HANDSHAKE." << endl;
                return false;
            case PacketType::UNDEFINED:
                cout << "Packet type is undefined: " << static_cast<uint32_t>(packet->packetType) << endl;
                return false;
            default: ;
        }
    }
}

bool authRevokeUser(const Serial &serial, const string &username) {
    if (!serial.writePacket(AuthPacket(AuthPacket::AuthType::REVOKE_USER, username)))
        return false;

    while (true) {
        cout << "Waiting for a packet..." << endl;
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 100);
            packet = serial.readPacket();
        }

        switch (packet->packetType) {
            case PacketType::DEBUG: {
                const auto &debugPacket = DebugPacket::unpack(packet);
                cout << "Packet type is DEBUG. Message: " << debugPacket->message << endl;
            }
            break;
            case PacketType::AUTH: {
                cout << "Packet is AUTH." << endl;
                const auto &authPacket = AuthPacket::unpack(packet);
                if (authPacket == nullptr) {
                    cerr << "Auth packet is nullptr." << endl;
                    return false;
                }
                switch (authPacket->authType) {
                    default: ;
                        return false;
                    case AuthPacket::AuthType::CHECK_USER:
                        cout << "Packet is AuthType::CHECK_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::CHECK_USER_RESPONSE:
                        cout << "Packet is AuthType::CHECK_USER_RESPONSE: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::SET_USER:
                        cout << "Packet is AuthType::SET_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::SET_USER_RESPONSE:
                        cout << "Packet is AuthType::SET_USER_RESPONSE: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::REVOKE_USER:
                        cout << "Packet is AuthType::REVOKE_USER: " << authPacket->payload << endl;
                        break;
                    case AuthPacket::AuthType::REVOKE_USER_RESPONSE:
                        cout << "Packet is AuthType::REVOKE_USER_RESPONSE: " << authPacket->payload << endl;
                        return authPacket->payload == "OK";
                }
                break;
            }
            case PacketType::HANDSHAKE:
                cout << "Packet is HANDSHAKE." << endl;
                return false;
            case PacketType::UNDEFINED:
                cout << "Packet type is undefined: " << static_cast<uint32_t>(packet->packetType) << endl;
                return false;
            default: ;
        }
    }
}

int main() {
    cout << "Connecting to " << path << "... ";
    Serial serial(path, BaudRate::BR_1152000);
    if (!serial.open()) {
        cout << "Failed." << endl;
        return 1;
    }
    cout << "Success." << endl;

    cout << "Sending handshake: " << endl;
    if (handshake(serial))
        cout << "Successfully handshake. " << endl;
    else {
        cout << "Failed handshaked. " << endl;
        return -1;
    }

    cout << "Setting auth..." << endl;
    if (authSetUser(serial, "test_username"))
        cout << "Succesfully set user. " << endl;
    else {
        cout << "Failed to set user. " << endl;
        return -1;
    }


    cout << "Checking auth..." << endl;
    if (authCheckUser(serial, "test_username"))
        cout << "Succesfully checked user. " << endl;
    else {
        cout << "Failed checking user. " << endl;
        return -1;
    }

    cout << "Revoking auth..." << endl;
    if (authRevokeUser(serial, "test_username"))
        cout << "Succesfully removed user. " << endl;
    else {
        cout << "Failed removing user. " << endl;
        return -1;
    }

    return 0;
}
