#ifndef CONTROL_H
#define CONTROL_H
#include <esp32-hal-gpio.h>

namespace control {
    namespace button::main {
        constexpr auto pin = GPIO_NUM_0;
        constexpr auto mode = INPUT_PULLUP;

        inline bool isPressed() {
            return !digitalRead(pin);
        }

        inline void init() {
            pinMode(pin, mode);
        }
    }


    namespace led::main {
        constexpr auto pin = LED_BUILTIN;

        inline void init() {
            pinMode(pin, INPUT | OUTPUT);
        }

        inline void on() {
            digitalWrite(pin, HIGH);
        }

        inline void off() {
            digitalWrite(pin, LOW);
        }

        inline void flipState() {
            digitalWrite(pin, !digitalRead(pin));
        }
    }

    namespace serial::main {
        constexpr auto baud = 115200;
    }
}

#endif //CONTROL_H
