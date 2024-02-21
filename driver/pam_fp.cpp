#include "serial/serial.h"
#ifndef PAM_MODULE_H
#define PAM_MODULE_H

#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <iostream>

/* expected hook */
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << std::endl << "RUNNING pam_sm_setcred" << std::endl;
    if (flags & PAM_DELETE_CRED)
        std::cout << "PAM_DELETE_CRED" << std::endl;
    else if (flags & PAM_ESTABLISH_CRED)
        std::cout << "PAM_ESTABLISH_CRED" << std::endl;
    else
        return PAM_SUCCESS;

    const char *username;
    if (const auto &retval = pam_get_user(pamh, &username, "Username: "); retval != PAM_SUCCESS)
        return retval;

    constexpr auto authDevicePath = "/dev/serial/by-id/usb-WEMOS.CC_LOLIN-S2-MINI_0-if00";

    Serial serial(authDevicePath, BaudRate::BR_115200);
    if (!serial.open()) {
        return PAM_CRED_UNAVAIL;
    }

    /* HANDSHAKE */
    if (const bool success = serial.writePacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN)); !success)
        return PAM_CRED_UNAVAIL;

    int retries = 30;
    bool handshaked = false;
    while (retries--) {
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 100); // 10ms;
            packet = serial.readPacket();
        }
        switch (packet->packetType) {
            default:
                break;
            case PacketType::HANDSHAKE: {
                if (const auto &handshakePacket = HandshakePacket::unpack(packet);
                    handshakePacket->stage == HandshakePacket::HandshakeStage::SYN_ACK) {
                    handshaked = true;
                    if (flags & PAM_DELETE_CRED) {
                        if (!serial.writePacket(AuthPacket(AuthPacket::AuthType::REMOVE_USER, username)))
                            return PAM_CRED_UNAVAIL;
                    } else if (flags & PAM_ESTABLISH_CRED)
                        if (!serial.writePacket(AuthPacket(AuthPacket::AuthType::SET_USER, username)))
                            return PAM_CRED_UNAVAIL;
                }
            }
            break;
            case PacketType::AUTH:
                if (!handshaked)
                    return PAM_CRED_UNAVAIL;
                switch (const auto &authPacket = AuthPacket::unpack(packet); authPacket->authType) {
                    default:
                        return PAM_CRED_UNAVAIL;
                    case AuthPacket::AuthType::SET_USER_RESPONSE:
                    case AuthPacket::AuthType::REVOKE_USER_RESPONSE:
                        if (authPacket->payload == "OK")
                            return PAM_SUCCESS;
                        return PAM_CRED_ERR;
                }
        }
    }

    return PAM_CRED_UNAVAIL;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << "[PAM] Acct mgmt" << std::endl;
    return PAM_SUCCESS;
}

/* expected hook, this is where custom stuff happens */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << std::endl << "[PAM] RUNNING pam_sm_authenticate" << std::endl;

    // pam_get_authtok()  // caching

    const char *username;
    if (const auto &retval = pam_get_user(pamh, &username, "Username: "); retval != PAM_SUCCESS)
        return retval;

    constexpr auto authDevicePath = "/dev/serial/by-id/usb-WEMOS.CC_LOLIN-S2-MINI_0-if00";

    Serial serial(authDevicePath, BaudRate::BR_115200, "[PAM] ");
    if (!serial.open()) {
        return PAM_AUTHINFO_UNAVAIL;
    }

    /* HANDSHAKE */
    if (const bool success = serial.writePacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN)); !success)
        return PAM_AUTHINFO_UNAVAIL;


    int retries = 30;
    bool handshaked = false;
    while (retries--) {
        auto packet = serial.readPacket();
        while (packet == nullptr) {
            constexpr auto oneSecond = 1000000;
            usleep(oneSecond / 100); // 10ms;
            packet = serial.readPacket();
        }
        switch (packet->packetType) {
            default:
                break;
            case PacketType::HANDSHAKE:
                if (const auto &handshakePacket = HandshakePacket::unpack(packet);
                    handshakePacket->stage == HandshakePacket::HandshakeStage::SYN_ACK) {
                    handshaked = true;
                    if (!serial.writePacket(AuthPacket(AuthPacket::AuthType::CHECK_USER, username)))
                        return PAM_AUTHINFO_UNAVAIL;
                }
                break;
            case PacketType::AUTH: {
                if (!handshaked)
                    return PAM_AUTHINFO_UNAVAIL;
                const auto &authPacket = AuthPacket::unpack(packet);
                std::cout << "[PAM] " << authPacket->str() << std::endl;
                switch (authPacket->authType) {
                    default:
                        return PAM_AUTHINFO_UNAVAIL;
                    case AuthPacket::AuthType::CHECK_USER_RESPONSE:
                        if (authPacket->payload == "VERIFIED")
                            return PAM_SUCCESS;
                        return PAM_USER_UNKNOWN;
                }
            }
        }
    }

    return PAM_MAXTRIES;
}

#endif //PAM_MODULE_H
