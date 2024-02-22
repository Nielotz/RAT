//https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/USBCDC.cpp
#include <Adafruit_TinyUSB.h>
#include <mutex>
#include <USBCDC.h>

#include <sfm.hpp>

#include "control.h"
#include "packet.h"
#include "usb_connection.h"

#define FP_UART_NUM UART_NUM_1
#define FP_UART_RX GPIO_NUM_4  // black
#define FP_UART_TX GPIO_NUM_5  // yellow
#define FP_UART_IRQ GPIO_NUM_6  // blue
#define FP_UART_VCC GPIO_NUM_7  // green

SFM_Module fingerPrintScanner(FP_UART_VCC, FP_UART_IRQ, FP_UART_TX, FP_UART_RX);

void IRAM_ATTR sfmPinInt1() {
    fingerPrintScanner.pinInterrupt();
}

auto &usbSerial = Serial;

uint8_t constexpr desc_hid_report[] = {TUD_HID_REPORT_DESC_MOUSE()};
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_MOUSE, 2, false);

void setup() {
    control::main::led::init();
    control::main::led::on();
    control::main::button::init();

    if (fingerPrintScanner.isConnected()) {
        fingerPrintScanner.setPinInterrupt(sfmPinInt1);
        fingerPrintScanner.setRingColor(SFM_RING_BLUE, SFM_RING_OFF);
    }
    usb_hid.begin();
}

struct Stage {
    enum {
        LOOP_INIT,

        MAIN_LOOP,
        PC_MESSAGE,

        ILLEGAL,
        // Safeguard before stage++ to EXIT.
        EXIT,
    };
};

bool handleHanshakePacket(UsbConnection &pc, const std::unique_ptr<Packet> &packet) {
    pc.sendPacket(DebugPacket("Packet is a handshake."));
    const auto &handshakePacket = HandshakePacket::unpack(packet);
    pc.sendPacket(DebugPacket(handshakePacket->str()));

    switch (handshakePacket->stage) {
        case HandshakePacket::HandshakeStage::SYN:
            pc.syn = 0;
            return pc.sendPacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN_ACK, pc.syn++));
        case HandshakePacket::HandshakeStage::SYN_ACK:
            pc.sendPacket(DebugPacket("uC side SYN_ACK not supported"));
            return false;
        case HandshakePacket::HandshakeStage::ACK:
            pc.sendPacket(DebugPacket("Handshake ACK OK"));
            return true;
        case HandshakePacket::HandshakeStage::INVALID:
        default:
            pc.sendPacket(DebugPacket("Received invalid handshake"));
            return false;
    }
}

enum class AuthStage {
    NONE,
    REGISTER,
    REGISTER_STAGE_1,
    REGISTER_STAGE_1_RELEASE,
    REGISTER_STAGE_2,
    REGISTER_STAGE_2_RELEASE,
    REGISTER_STAGE_3,

    CHECK,
    CHECK_STAGE_1,
    DELETE,
};

void handleNetwork(UsbConnection &pc, AuthStage &authStage, const int packetsPerIteration = 20) {
    for (auto i = 0; i < packetsPerIteration; i++) {
        const std::unique_ptr<Packet> packet = pc.receivePacket();
        if (packet == nullptr)
            break; // No available packets.

        // ReSharper disable once CppExpressionWithoutSideEffects
        pc.sendPacket(DebugPacket("Received packet: " + packet->str()));
        switch (packet->packetType) {
            case PacketType::HANDSHAKE: {
                if (handleHanshakePacket(pc, packet) and pc.syn != static_cast<uint32_t>(-1))
                    pc.sendPacket(DebugPacket("Handshaked succesfully."));
                break;
            }
            case PacketType::AUTH: {
                pc.sendPacket(DebugPacket("Packet is AUTH"));
                if (pc.syn == static_cast<uint32_t>(-1)) {
                    pc.sendPacket(DebugPacket("Cannot handle AUTH while handshake is not set."));
                    break;
                }
                if (!fingerPrintScanner.isConnected()) {
                    pc.sendPacket(DebugPacket("Fingerprint scanner is not connected."));
                    break;
                }

                const auto &authPacket = AuthPacket::unpack(packet);
                pc.sendPacket(DebugPacket(std::string(std::string("Payload: ") + authPacket->payload)));
                switch (authPacket->authType) {
                    default: ;
                        break;
                    case AuthPacket::AuthType::SET_USER:
                        pc.sendPacket(DebugPacket("Packet is SET_USER."));
                        authStage = AuthStage::REGISTER;
                        break;
                    case AuthPacket::AuthType::CHECK_USER:
                        pc.sendPacket(DebugPacket("Packet is CHECK_USER."));
                        authStage = AuthStage::CHECK;
                        break;
                    case AuthPacket::AuthType::REMOVE_USER:
                        pc.sendPacket(DebugPacket("Packet is REMOVE_USER."));
                        authStage = AuthStage::DELETE;
                        break;
                }
            }
            break;
            case PacketType::DEBUG:
            case PacketType::UNDEFINED:
            default: ;
        }
    }
}

void handleAuth(const UsbConnection &pc, AuthStage &authStage) {
    switch (authStage) {
        default: ;
            break;

        case AuthStage::REGISTER: {
            fingerPrintScanner.enable();
            pc.sendPacket(DebugPacket("[AUTH 0/3] Rozpoczęto proces dodawania odcisku palca."));
            fingerPrintScanner.setRingColor(SFM_RING_PURPLE, SFM_RING_BLUE, 100);

            authStage = AuthStage::REGISTER_STAGE_1;
            pc.sendPacket(DebugPacket("[AUTH 1/3] Przyłóż palec do czytnika."));
        }

        case AuthStage::REGISTER_STAGE_1: {
            if (!fingerPrintScanner.isTouched())
                break;
            fingerPrintScanner.setRingColor(SFM_RING_PURPLE);
            if (fingerPrintScanner.register_3c3r_1st() != SFM_ACK_SUCCESS) {
                fingerPrintScanner.setRingColor(SFM_RING_RED);
                pc.sendPacket(DebugPacket("[AUTH 1/3] Wystąpił błąd."));
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::SET_USER_RESPONSE, "FAIL"));
                authStage = AuthStage::NONE;
                break;
            }
            fingerPrintScanner.setRingColor(SFM_RING_GREEN);
            pc.sendPacket(DebugPacket("[AUTH 1/3] OK. Odsuń palec od czytnika."));
            authStage = AuthStage::REGISTER_STAGE_1_RELEASE;
        }

        case AuthStage::REGISTER_STAGE_1_RELEASE: {
            if (fingerPrintScanner.isTouched())
                break;
            authStage = AuthStage::REGISTER_STAGE_2;
            pc.sendPacket(DebugPacket("[AUTH 2/3] Przyłóż palec do czytnika."));
            fingerPrintScanner.setRingColor(SFM_RING_PURPLE, SFM_RING_CYAN, 100);
        }

        case AuthStage::REGISTER_STAGE_2: {
            if (!fingerPrintScanner.isTouched())
                break;
            fingerPrintScanner.setRingColor(SFM_RING_PURPLE);
            if (fingerPrintScanner.register_3c3r_2nd() != SFM_ACK_SUCCESS) {
                fingerPrintScanner.setRingColor(SFM_RING_RED);
                pc.sendPacket(DebugPacket("[AUTH 2/3] Wystąpił błąd."));
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::SET_USER_RESPONSE, "FAIL"));
                authStage = AuthStage::NONE;
                break;
            }
            fingerPrintScanner.setRingColor(SFM_RING_GREEN);
            pc.sendPacket(DebugPacket("[AUTH 2/3] OK. Odsuń palec od czytnika."));
            authStage = AuthStage::REGISTER_STAGE_2_RELEASE;
        }

        case AuthStage::REGISTER_STAGE_2_RELEASE: {
            if (fingerPrintScanner.isTouched())
                break;
            authStage = AuthStage::REGISTER_STAGE_3;
            pc.sendPacket(DebugPacket("[AUTH 3/3] Przyłóż palec do czytnika."));
            fingerPrintScanner.setRingColor(SFM_RING_PURPLE, SFM_RING_GREEN, 100);
        }

        case AuthStage::REGISTER_STAGE_3: {
            if (!fingerPrintScanner.isTouched())
                break;
            fingerPrintScanner.setRingColor(SFM_RING_PURPLE);
            uint16_t uid = 0;
            if (fingerPrintScanner.register_3c3r_3rd(uid) != SFM_ACK_SUCCESS or uid == 0) {
                fingerPrintScanner.setRingColor(SFM_RING_RED);
                pc.sendPacket(DebugPacket("[AUTH 3/3] Wystąpił błąd."));
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::SET_USER_RESPONSE, "FAIL"));
                authStage = AuthStage::NONE;
                break;
            }

            fingerPrintScanner.setRingColor(SFM_RING_GREEN);
            using std::string;
            using std::to_string;
            pc.sendPacket(DebugPacket(string("[AUTH 3/3] OK. uuid: ") + to_string(static_cast<uint32_t>(uid))));
            pc.sendPacket(AuthPacket(AuthPacket::AuthType::SET_USER_RESPONSE, "OK"));

            authStage = AuthStage::NONE;
            fingerPrintScanner.setRingColor(SFM_RING_OFF);
            fingerPrintScanner.disable();
        }
        break;

        case AuthStage::CHECK: {
            fingerPrintScanner.enable();
            fingerPrintScanner.setRingColor(SFM_RING_YELLOW);
            pc.sendPacket(DebugPacket("[AUTH] Przyłóż palec do czytnika."));
            authStage = AuthStage::CHECK_STAGE_1;
        }

        case AuthStage::CHECK_STAGE_1: {
            if (!fingerPrintScanner.isTouched())
                break;

            if (uint16_t uid = 0; fingerPrintScanner.recognition_1vN(uid) != SFM_ACK_SUCCESS or uid == 0) {
                fingerPrintScanner.setRingColor(SFM_RING_RED);
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::CHECK_USER_RESPONSE, "REJECTED"));
            } else {
                fingerPrintScanner.setRingColor(SFM_RING_GREEN);
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::CHECK_USER_RESPONSE, "VERIFIED"));
            }
            fingerPrintScanner.disable();
            authStage = AuthStage::NONE;
        }
        break;

        case AuthStage::DELETE: {
            fingerPrintScanner.enable();
            if (fingerPrintScanner.deleteAllUser() != SFM_ACK_SUCCESS)
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::REMOVE_USER_RESPONSE, "FAIL"));
            else
                pc.sendPacket(AuthPacket(AuthPacket::AuthType::REMOVE_USER_RESPONSE, "OK"));
            authStage = AuthStage::NONE;
            fingerPrintScanner.disable();
        }
    }
}

void handleMouse() {
    using namespace control::mouse;
    const struct {
        const int &x, &y;
    } mouseMovement = {
                .x = -move_left_button::isPressed() + move_right_button::isPressed(),
                .y = -move_down_button::isPressed() + move_up_button::isPressed(),
            };

    if (mouseMovement.x || mouseMovement.y) {
        // Update mouse.
        if (TinyUSBDevice.suspended()) {
            // Wake up host if we are in suspend mode and REMOTE_WAKEUP feature is enabled by host
            TinyUSBDevice.remoteWakeup();
        }
        if (usb_hid.ready()) {
            constexpr auto delta = 4;
            constexpr uint8_t reportId = 0; // no ID
            usb_hid.mouseMove(reportId,
                              static_cast<int8_t>(mouseMovement.x * delta),
                              static_cast<int8_t>(mouseMovement.y * delta));
        }
    }

    if (control::main::button::isPressed()) {
        usb_hid.mouseMove(0,
                          5,
                          5);
    }
}

void loop() {
    static int stage = Stage::LOOP_INIT;
    static UsbConnection pc(usbSerial);
    static auto authStage = AuthStage::NONE;
    static int cycleTime = 10;

    switch (stage) {
        case Stage::LOOP_INIT: {
            UsbConnection::debugUsbConnection = &pc;
            pc.sendPacket(DebugPacket("Starting main loop."));
            stage = Stage::MAIN_LOOP;
        }

        case Stage::MAIN_LOOP: {
            if (usb_hid.isValid() and TinyUSBDevice.mounted())
                handleMouse();

            if (pc.usb) {
                static std::once_flag flag1;
                std::call_once(flag1, []() { pc.setUsbCallback(); });

                handleNetwork(pc, authStage);
                handleAuth(pc, authStage);

                control::main::led::on();
            } else
                control::main::led::off();
        }
        break;
        case Stage::EXIT:
            // ReSharper disable once CppExpressionWithoutSideEffects
            pc.sendPacket(DebugPacket("Exiting."));
            return;
        case Stage::ILLEGAL:
            break;
        default:
            // ReSharper disable once CppExpressionWithoutSideEffects
            pc.sendPacket(DebugPacket("Illegal loop stage."));
            cycleTime = 0;
            stage = Stage::EXIT;
    }
    delay(cycleTime);
    // vTaskDelay(cycleTime);
}
