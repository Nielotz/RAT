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

bool handle_hanshake(UsbConnection &pc, const std::unique_ptr<Packet> &packet) {
    const auto &handshakePacket = HandshakePacket::unpack(packet);
    pc.sendPacket(DebugPacket(handshakePacket->str()).pack());
    switch (handshakePacket->stage) {
        case HandshakePacket::HandshakeStage::SYN:
            pc.syn = 0;
            return pc.sendPacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN_ACK, pc.syn++).pack());
        case HandshakePacket::HandshakeStage::SYN_ACK:
            return pc.sendPacket(DebugPacket("uC side SYN_ACK not supported").pack());
        case HandshakePacket::HandshakeStage::ACK:
            return pc.sendPacket(DebugPacket("Handshake ACK OK").pack());
        case HandshakePacket::HandshakeStage::INVALID:
        default:
            return pc.sendPacket(DebugPacket("Received invalid handshake").pack());
    }
}

void handleUsbEvent(UsbConnection &pc) {
    if (!pc.usb)
        return;
    // pc.setCallback();

    const std::unique_ptr<Packet> packet = pc.receivePacket();
    if (packet == nullptr)
        return;

    pc.sendPacket(DebugPacket("Received packet: " + packet->str()).pack());
    switch (packet->packetType) {
        case PacketType::HANDSHAKE: {
            pc.sendPacket(DebugPacket("Packet is a handshake.").pack());
            const bool success = handle_hanshake(pc, packet);
            pc.sendPacket(DebugPacket(std::string("Handled handshake: ") + (success ? "success" : "failed")).pack());
        }

            break;
        case PacketType::DEBUG:
        case PacketType::UNDEFINED:
        default: ;
    }
}

void loop() {
    static int stage = Stage::LOOP_INIT;
    static UsbConnection pc(usbSerial);
    bool failed = false;

    switch (stage) {
        case Stage::LOOP_INIT:
            UsbConnection::debugUsbConnection = &pc;
            failed = pc.sendPacket(DebugPacket("Starting main loop.").pack());

            cycleTime = 10;
            stage = Stage::MAIN_LOOP;

        case Stage::MAIN_LOOP:
            handleMainButton(pc);
            handleUsbEvent(pc);
            if (pc.usb)
                control::main::led::on();
            else
                control::main::led::off();

            break;
        case Stage::EXIT:
            failed = pc.sendPacket(DebugPacket("Exiting.").pack());
            return;
        case Stage::ILLEGAL:
            break;
        default:
            failed = pc.sendPacket(DebugPacket("Illegal loop stage.").pack());
            cycleTime = 0;
            stage = Stage::EXIT;
    }
    delay(cycleTime);
    // vTaskDelay(cycleTime);
}
