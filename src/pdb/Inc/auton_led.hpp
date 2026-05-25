#pragma once

#include <hw/pin.hpp>

namespace mrover {
    class AutonLED {
    private:
        Pin m_red_pin;
        Pin m_green_pin;
        Pin m_blue_pin;

        bool m_red{true};
        bool m_green{false};
        bool m_blue{false};
        bool m_blinking{false};
        bool m_on{true};

        void change_all_pins() {
            m_red_pin.write(m_red && m_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
            m_green_pin.write(m_green && m_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
            m_blue_pin.write(m_blue && m_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }

    public:
        AutonLED() = default;

        AutonLED(Pin red_pin, Pin green_pin, Pin blue_pin)
            : m_red_pin(red_pin), m_green_pin(green_pin), m_blue_pin(blue_pin) {
            change_all_pins();
        }

        void change_state(bool red, bool green, bool blue, bool blinking) {
            if (m_blinking != blinking)
                m_on = true;

            m_red = red;
            m_green = green;
            m_blue = blue;
            m_blinking = blinking;

            change_all_pins();
        }

        void blink() {
            if (m_blinking) {
                change_all_pins();
                m_on = !m_on;
            }
        }
    };
} // namespace mrover
