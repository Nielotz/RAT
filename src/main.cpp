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


void loop() {
    static int stage = Stage::LOOP_INIT;
    static UsbConnection pc(usbSerial);
    bool failed = false;

    switch (stage) {
        case Stage::LOOP_INIT:
            UsbConnection::debugUsbConnection = &pc;
            failed = pc.sendPacket(DebugPacket("Starting main loop.").pack());
            if (failed) {
                control::main::led::flipState();
                pc.usb.begin();
                cycleTime = 100;
            } else {
                cycleTime = 10;
                stage = Stage::MAIN_LOOP;
            }

        case Stage::MAIN_LOOP:
            handleMainButton(pc);
            pc.handleUsb();

            if (pc.usb)
                control::main::led::on();
            else
                control::main::led::off();

            break;
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
