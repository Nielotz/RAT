//https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/USBCDC.cpp
#include <Adafruit_TinyUSB.h>
#include <USBCDC.h>

#include <sfm.hpp>

#include "control.h"
#include "packet.h"
#include "usb_connection.h"

#define FP_UART_NUM UART_NUM_1
#define FP_UART_RX GPIO_NUM_4
#define FP_UART_TX GPIO_NUM_5
#define FP_UART_IRQ GPIO_NUM_6
#define FP_UART_VCC GPIO_NUM_7

SFM_Module fingerPrintScanner(FP_UART_VCC, FP_UART_IRQ, FP_UART_TX, FP_UART_RX);

void sfmPinInt1() {
    fingerPrintScanner.pinInterrupt();
}

auto &usbSerial = Serial;

int cycleTime = 1000;

void setup() {
    control::main::led::init();
    control::main::led::on();
    control::main::button::init();

    if (fingerPrintScanner.isConnected()) {
        fingerPrintScanner.setPinInterrupt(sfmPinInt1);
        fingerPrintScanner.setRingColor(SFM_RING_BLUE, SFM_RING_OFF);
    }
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

void handleMainButton(const UsbConnection &pc) {
    if (control::main::button::isPressed()) {
        // ReSharper disable once CppExpressionWithoutSideEffects
        pc.sendPacket(DebugPacket("Button pressed."));
        control::main::led::flipState();
    }
}

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

void loop() {
    static int stage = Stage::LOOP_INIT;
    static UsbConnection pc(usbSerial);
    constexpr int packetsPerIteration = 20;

    switch (stage) {
        case Stage::LOOP_INIT: {
            UsbConnection::debugUsbConnection = &pc;
            if (!pc.sendPacket(DebugPacket("Starting main loop."))) {
                control::main::led::flipState();
                pc.usb.begin();
                cycleTime = 100;
            } else {
                cycleTime = 10;
                stage = Stage::MAIN_LOOP;
            }
        }

        case Stage::MAIN_LOOP: {
            handleMainButton(pc);
            // handleMouse(pc);

            if (!pc.usb) {
                control::main::led::off();
                break;
            }
            control::main::led::on();
            pc.setUsbCallback();

            for (auto i = 0; i < packetsPerIteration; i++) {
                const std::unique_ptr<Packet> packet = pc.receivePacket();
                if (packet == nullptr)
                    break; // No available packets.

                // ReSharper disable once CppExpressionWithoutSideEffects
                pc.sendPacket(DebugPacket("Received packet: " + packet->str()));
                switch (packet->packetType) {
                    case PacketType::HANDSHAKE: {
                        const bool handshaked = handleHanshakePacket(pc, packet);
                        if (handshaked and pc.syn != static_cast<uint32_t>(-1))
                            pc.sendPacket(DebugPacket("Handshaked succesfully."));
                        break;
                    }
                    case PacketType::AUTH: {
                        const auto &authPacket = AuthPacket::unpack(packet);
                        pc.sendPacket(DebugPacket("Packet is AUTH. Payload: " + authPacket->payload));
                        switch (authPacket->authType) {
                            default: ;
                                break;
                            case AuthPacket::AuthType::SET_USER:
                                pc.sendPacket(DebugPacket("Packet is SET_USER."));
                                if (!fingerPrintScanner.isConnected()) {
                                    pc.sendPacket(DebugPacket("Fingerprint scanner is not connected."));
                                    pc.sendPacket(AuthPacket(AuthPacket::AuthType::SET_USER_RESPONSE, "FAIL")
                                        );
                                }
                            case AuthPacket::AuthType::CHECK_USER:
                                pc.sendPacket(DebugPacket("Packet is CHECK_USER."));
                                pc.sendPacket(AuthPacket(AuthPacket::AuthType::CHECK_USER_RESPONSE,
                                                         "VERIFIED"));
                                break;
                        }
                    }
                    break;
                    case PacketType::DEBUG:
                    case PacketType::UNDEFINED:
                    default: ;
                }
            }


            break;
        }
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
