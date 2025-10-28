#ifndef CONTROL_H
#define CONTROL_H
#include <esp32-hal-gpio.h>

namespace control {
    namespace main {
        namespace button {
            constexpr auto pin = GPIO_NUM_0;
            constexpr auto mode = INPUT_PULLUP;

            inline bool isPressed() {
                return !digitalRead(pin);
            }

            inline void init() {
                pinMode(pin, mode);
            }
        }

        namespace led {
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
    }

    namespace mouse {
        namespace move_left_button {
            constexpr auto pin = GPIO_NUM_8;
            constexpr auto mode = INPUT;

            inline bool isPressed() {
                return !digitalRead(pin);
            }

            inline void init() {
                pinMode(pin, mode);
            }
        }

        namespace move_right_button {
            constexpr auto pin = GPIO_NUM_9;
            constexpr auto mode = INPUT;

            inline bool isPressed() {
                return !digitalRead(pin);
            }

            inline void init() {
                pinMode(pin, mode);
            }
        }

        namespace move_up_button {
            constexpr auto pin = GPIO_NUM_10;
            constexpr auto mode = INPUT;

            inline bool isPressed() {
                return !digitalRead(pin);
            }

            inline void init() {
                pinMode(pin, mode);
            }
        }

        namespace move_down_button {
            constexpr auto pin = GPIO_NUM_11;
            constexpr auto mode = INPUT;

            inline bool isPressed() {
                return !digitalRead(pin);
            }

            inline void init() {
                pinMode(pin, mode);
            }
        }
    }
}

#endif //CONTROL_H
