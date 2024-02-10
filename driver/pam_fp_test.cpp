#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <cstdio>
#include <iostream>

#include "serial/serial.h"


constexpr pam_conv conv = {
    misc_conv, // Defined in pam_misc.h
    nullptr
};

int main(const int argc, char *argv[]) {
    pam_handle_t *pamh = nullptr;
    int retval = pam_start("sudo", nullptr, &conv, &pamh);

    Serial serial("/dev/ttyACM0", BaudRate::BR_9600);
    serial.open();

    while (true) {
        std::cout << "Waiting for a packet..." << std::endl;
        const auto packet = serial.readPacket();
        switch (packet->packetType) {
            case PacketType::UNDEFINED:
                std::cout << "Packet type is UNDEFINED. Data: " << std::endl;
                break;
            case PacketType::DEBUG: {
                const auto debugPacket = DebugPacket::unpack(packet);
                std::cout << "Packet type is DEBUG. Message: " << debugPacket->message << std::endl;
            }
            break;
            case PacketType::HANDSHAKE: {
                const auto handshakePacket = HandshakePacket::unpack(packet);
                switch (handshakePacket->stage) {
                    case HandshakePacket::HandshakeStage::SYN_ACK:
                        std::cout << "Packet is SYN_ACK HANDSHAKE. ACK number: " << handshakePacket->ackNumber << std::endl;

                        break;
                    case HandshakePacket::HandshakeStage::SYN:
                        std::cout << "Packet is SYN HANDSHAKE. Not supported  on PC side." << std::endl;
                        break;
                    case HandshakePacket::HandshakeStage::ACK:
                        std::cout << "Packet is ACK HANDSHAKE. Not supported  on PC side." << std::endl;
                        break;
                    default:
                    case HandshakePacket::HandshakeStage::INVALID:
                        std::cout << "Packet is INVALID HANDSHAKE." << std::endl;
                        break;
                }
                std::cout << "Responding with the same packet... ";
                if (serial.writePacket(handshakePacket->pack()))
                    std::cout << "Success.";
                else
                    std::cout << "Failed.";

            }
            break;
            default: ;
        }
    }

    // Are the credentials correct?
    if (retval == PAM_SUCCESS) {
        std::cout << "Credentials accepted." << std::endl;
        retval = pam_authenticate(pamh, 0);
    }

    // Can the accound be used at this time?
    if (retval == PAM_SUCCESS) {
        std::cout << "Account is valid." << std::endl;
        retval = pam_acct_mgmt(pamh, 0);
    }

    // Did everything work?
    if (retval == PAM_SUCCESS) {
        std::cout << "Authenticated" << std::endl;
    } else {
        std::cout << "Not Authenticated" << std::endl;
    }

    // close PAM (end session)
    if (pam_end(pamh, retval) != PAM_SUCCESS) {
        pamh = nullptr;
        std::cout << "check_user: failed to release authenticator" << std::endl;
        exit(1);
    }

    return retval == PAM_SUCCESS ? 0 : 1;
}
