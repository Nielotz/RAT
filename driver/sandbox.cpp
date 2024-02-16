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


int main() {
    using std::cout;
    using std::endl;

    pam_handle_t *pamh = nullptr;
    cout << "Initializing PAM transaction...";
    int retval = pam_start("sudo", nullptr, &conv, &pamh);


    cout << "Connecting to " << path << "... ";
    Serial serial(path, BaudRate::BR_19200);
    if (!serial.open()) {
        cout << "Failed." << endl;
        return 1;
    }
    cout << "Success." << endl;

    cout << "Sending handshake: " << endl;
    serial.writePacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN).pack());

    while (true) {
        cout << "Waiting for a packet..." << endl;
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 2);
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
                        break;

                    case HandshakePacket::HandshakeStage::SYN:
                        cout << "Packet is SYN HANDSHAKE. Not supported  on PC side." << endl;
                        break;
                    case HandshakePacket::HandshakeStage::ACK:
                        cout << "Packet is ACK HANDSHAKE. Not supported  on PC side." << endl;
                        break;
                    default:
                    case HandshakePacket::HandshakeStage::INVALID:
                        cout << "Packet is INVALID HANDSHAKE." << endl;
                        break;
                }

                break;
            case PacketType::UNDEFINED:
                cout << "Packet type is undefined: " << static_cast<uint32_t>(packet->packetType) << endl;
                break;
            default: ;
        }
    }


    // Are the credentials correct?
    if (retval == PAM_SUCCESS) {
        cout << "Credentials accepted." << endl;
        retval = pam_authenticate(pamh, 0);
    }

    // Can the accound be used at this time?
    if (retval == PAM_SUCCESS) {
        cout << "Account is valid." << endl;
        retval = pam_acct_mgmt(pamh, 0);
    }

    // Did everything work?
    if (retval == PAM_SUCCESS) {
        cout << "Authenticated" << endl;
    } else {
        cout << "Not Authenticated" << endl;
    }

    // close PAM (end session)
    if (pam_end(pamh, retval) != PAM_SUCCESS) {
        pamh = nullptr;
        cout << "check_user: failed to release authenticator" << endl;
        exit(1);
    }

    return retval == PAM_SUCCESS ? 0 : 1;
}
