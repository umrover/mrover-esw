#pragma once

#include <cstdint>

#include "main.h"

namespace mrover {

    class Pin {
    public:
        Pin() = default;

        Pin(GPIO_TypeDef* port, uint16_t pin) : m_port{port}, m_pin{pin} {}

        [[nodiscard]] auto read() const -> GPIO_PinState {
            return HAL_GPIO_ReadPin(m_port, m_pin);
        }

        auto write(GPIO_PinState val) const -> void {
            HAL_GPIO_WritePin(m_port, m_pin, val);
        }

        auto reset() const -> void {
            HAL_GPIO_WritePin(m_port, m_pin, GPIO_PIN_RESET);
        }

        auto set() const -> void { HAL_GPIO_WritePin(m_port, m_pin, GPIO_PIN_SET); }

    private:
        GPIO_TypeDef* m_port{};
        uint16_t m_pin{};
    };
} // namespace mrover
