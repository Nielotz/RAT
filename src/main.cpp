#include "Adafruit_TinyUSB.h"
#include <HardwareSerial.h>
#include <USBCDC.h>
#include "driver/uart.h"

#include <sfm.hpp>

#include "control.h"
#include "debug.h"

/* FP */
// Serial
#define FP_UART_NUM UART_NUM_1
#define FP_UART_RX GPIO_NUM_4
#define FP_UART_TX GPIO_NUM_5
#define FP_UART_IRQ GPIO_NUM_6
#define FP_UART_VCC GPIO_NUM_7
SFM_Module fingerprint(FP_UART_VCC, FP_UART_IRQ, FP_UART_TX, FP_UART_RX);

uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_MOUSE()
};
// USB HID object. For ESP32 these values cannot be changed after this declaration
// desc report, desc len, protocol, interval, use out endpoint
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_MOUSE, 2, false);

auto &debugSerial = Serial;
int cycleTime = 100;

void sfmPinInt1() {
    fingerprint.pinInterrupt();
    DEBUG_MSG(debugSerial, "FP touch detected.");
}


void setup() {
    auto &debugSerial = Serial;

    struct Stage {
        enum {
            LED_INIT,
            BUTTON_INIT,

            SERIAL_INIT,
            SERIAL_CHECK,

            FP_SERIAL_CHECK,
            FP_SERIAL_CONFIG,

            USB_HID_INIT,
            USB_HID_CHECK,

            ILLEGAL,
            // Safeguard before stage++ to EXIT.
            DONE,
        };
    };
    int stage = Stage::LED_INIT;
    while (true) {
        switch (stage) {
            case Stage::LED_INIT:
                control::led::main::init();
                control::led::main::on();
                stage++;
            case Stage::BUTTON_INIT:
                control::button::main::init();
                stage++;

            case Stage::SERIAL_INIT:
                debugSerial.begin(115200);
                stage++;
                cycleTime = 100;

            case Stage::SERIAL_CHECK:
                if (!debugSerial) {
                    control::led::main::flipState();
                    break;
                }
                stage++;
                DEBUG_MSG(debugSerial, "Default serial initialized.");
                control::led::main::on();

                cycleTime = 1000;
            case Stage::FP_SERIAL_CHECK: {
                if (!fingerprint.isConnected()) {
                    control::led::main::flipState();
                    DEBUG_MSG(debugSerial, "Fingerprint is not connected.");
                    break;
                }
                stage++;
                DEBUG_MSG(debugSerial, "FP connected.");
                control::led::main::on();
            }

            case Stage::FP_SERIAL_CONFIG:
                DEBUG_MSG(debugSerial, "Configuring interrupt...");
                fingerprint.setPinInterrupt(sfmPinInt1);
                fingerprint.setRingColor(SFM_RING_CYAN);
                stage++;
                cycleTime = 200;
                stage++;

            case Stage::USB_HID_INIT:
                DEBUG_MSG(debugSerial, "Initializing USB HID...");
                usb_hid.begin();

                stage++;

            case Stage::USB_HID_CHECK:
                if (!usb_hid.isValid()) {
                    control::led::main::flipState();
                    DEBUG_MSG(debugSerial, "USB HID is invalid.");
                    break;
                }
                if (!TinyUSBDevice.mounted()) {
                    DEBUG_MSG(debugSerial, "USB HID is not mounted.");
                    control::led::main::flipState();
                    break;
                }
                DEBUG_MSG(debugSerial, "USB HID connected.");
                control::led::main::on();
                stage++;
                stage = Stage::DONE;
            case Stage::DONE:
                DEBUG_MSG(debugSerial, "Done setup.");
                cycleTime = 1000;
                return;
            default: ;
                DEBUG_MSG(debugSerial, "Invalid stage.");
                cycleTime = 50;
        }
        delay(cycleTime);
    }
}

struct Stage {
    enum {
        LOOP_INIT,

        WAIT_FOR_BUTTON_PRESS,
        HANDLE_BUTTON_PRESSED,

        ILLEGAL,
        // Safeguard before stage++ to EXIT.
        EXIT,
    };
};

int stage = Stage::LOOP_INIT;

void loop() {
    switch (stage) {
        case Stage::LOOP_INIT:
            DEBUG_MSG(debugSerial, "Starting main loop.");
            cycleTime = 10;
            stage++;
            break;

        case Stage::WAIT_FOR_BUTTON_PRESS:
            if (control::button::main::isPressed()) {
                DEBUG_MSG(debugSerial, "Button pressed.");
                stage = Stage::HANDLE_BUTTON_PRESSED;
            } else {
                break;
            }

        case Stage::HANDLE_BUTTON_PRESSED:
            // control::led::main::on();
            // Remote wakeup
            if (TinyUSBDevice.suspended()) {
                // Wake up host if we are in suspend mode and REMOTE_WAKEUP feature is enabled by host
                TinyUSBDevice.remoteWakeup();
            }
            if (usb_hid.ready()) {
                constexpr uint8_t report_id = 0; // no ID
                constexpr int8_t delta = 5;
                usb_hid.mouseMove(report_id, delta, delta); // right + down

                stage = Stage::WAIT_FOR_BUTTON_PRESS;
            }
            break;
        case Stage::ILLEGAL:
        default:
            DEBUG_MSG(debugSerial, "Illegal loop stage.");
        case Stage::EXIT:
            DEBUG_MSG(debugSerial, "Exiting.");
            return;
    }
    delay(cycleTime);
}
