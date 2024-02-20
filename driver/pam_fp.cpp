#include "serial/serial.h"
#ifndef PAM_MODULE_H
#define PAM_MODULE_H

#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <iostream>

/* expected hook */
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    std::cout << "Acct mgmt" << std::endl;
    return PAM_SUCCESS;
}

/* expected hook, this is where custom stuff happens */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    // pam_get_authtok()  // caching

    const char *username;
    if (const auto &retval = pam_get_user(pamh, &username, "Username: "); retval != PAM_SUCCESS)
        return retval;

    constexpr auto authDevicePath = "/dev/serial/by-id/usb-WEMOS.CC_LOLIN-S2-MINI_0-if00";

    Serial serial(authDevicePath, BaudRate::BR_115200);
    if (!serial.open()) {
        // Underlying authentication service can not retrieve authentication information.
        return PAM_AUTHINFO_UNAVAIL;
    }

    /* HANDSHAKE */
    if (const bool success = serial.writePacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN)); !success)
        return PAM_CONV_ERR; // Conversation error.

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
                        return PAM_CONV_ERR;
                }
                break;
            case PacketType::AUTH:
                if (!handshaked)
                    return PAM_AUTHTOK_ERR; /* Authentication token manipulation error */
                switch (const auto &authPacket = AuthPacket::unpack(packet); authPacket->authType) {
                    default:
                        return PAM_CONV_ERR;
                    case AuthPacket::AuthType::CHECK_USER_RESPONSE:
                        if (authPacket->payload == "VERIFIED")
                            return PAM_SUCCESS;
                        return PAM_USER_UNKNOWN;
                }
        }
    }

    // conversation function is event driven and data is not available yet
    return PAM_CONV_AGAIN;
}

#endif //PAM_MODULE_H
