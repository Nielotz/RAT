//https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/USBCDC.cpp
#include <Adafruit_TinyUSB.h>
#include <USBCDC.h>

#include <sfm.hpp>

#include "control.h"
#include "packet.h"
#include "usb_connection.h"

auto &usbSerial = Serial;

int cycleTime = 1000;

void setup() {
    control::main::led::init();
    control::main::led::on();
    control::main::button::init();
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
        pc.sendPacket(DebugPacket("Button pressed.").pack());
        control::main::led::flipState();
    }
}

bool handleHanshakePacket(UsbConnection &pc, const std::unique_ptr<Packet> &packet) {
    pc.sendPacket(DebugPacket("Packet is a handshake.").pack());
    const auto &handshakePacket = HandshakePacket::unpack(packet);
    pc.sendPacket(DebugPacket(handshakePacket->str()).pack());

    switch (handshakePacket->stage) {
        case HandshakePacket::HandshakeStage::SYN:
            pc.syn = 0;
            return pc.sendPacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN_ACK, pc.syn++).pack());
        case HandshakePacket::HandshakeStage::SYN_ACK:
            pc.sendPacket(DebugPacket("uC side SYN_ACK not supported").pack());
            return false;
        case HandshakePacket::HandshakeStage::ACK:
            pc.sendPacket(DebugPacket("Handshake ACK OK").pack());
            return true;
        case HandshakePacket::HandshakeStage::INVALID:
        default:
            pc.sendPacket(DebugPacket("Received invalid handshake").pack());
            return false;
    }
}

void loop() {
    static int stage = Stage::LOOP_INIT;
    static UsbConnection pc(usbSerial);
    constexpr int packetsPerIteration = 10;

    switch (stage) {
        case Stage::LOOP_INIT: {
            UsbConnection::debugUsbConnection = &pc;
            if (!pc.sendPacket(DebugPacket("Starting main loop.").pack())) {
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
                pc.sendPacket(DebugPacket("Received packet: " + packet->str()).pack());
                switch (packet->packetType) {
                    case PacketType::HANDSHAKE: {
                        const bool handshaked = handleHanshakePacket(pc, packet);
                        if (handshaked and pc.syn != static_cast<uint32_t>(-1))
                            pc.sendPacket(DebugPacket("Handshaked succesfully.").pack());
                        break;
                    }
                    case PacketType::AUTH: {
                        const auto &authPacket = AuthPacket::unpack(packet);
                        pc.sendPacket(DebugPacket("Packet is AUTH. Payload: " + authPacket->payload).pack());
                        switch (authPacket->authType) {
                            default: ;
                                break;
                            case AuthPacket::AuthType::CHECK_USER:
                                pc.sendPacket(DebugPacket("Packet is CHECK_USER.").pack());
                                pc.sendPacket(AuthPacket(AuthPacket::AuthType::CHECK_USER_RESPONSE, "VERIFIED").pack());
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
            pc.sendPacket(DebugPacket("Exiting.").pack());
            return;
        case Stage::ILLEGAL:
            break;
        default:
            // ReSharper disable once CppExpressionWithoutSideEffects
            pc.sendPacket(DebugPacket("Illegal loop stage.").pack());
            cycleTime = 0;
            stage = Stage::EXIT;
    }
    delay(cycleTime);
    // vTaskDelay(cycleTime);
}
