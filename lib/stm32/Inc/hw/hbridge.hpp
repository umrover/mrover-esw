#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>

#include <units.hpp>
#include <util.hpp>

#include "main.h"
#include "pin.hpp"

namespace mrover {

    /**
     * \brief Interface to MRover H-Bridge circuit
     */
    class HBridge {
        Pin m_direction_pin{};
        TIM_HandleTypeDef* m_timer{};
        std::uint32_t m_channel{};
        Percent m_max_pwm{};
        bool m_is_inverted = false;

    public:
        HBridge() = default;

        explicit HBridge(TIM_HandleTypeDef* timer, std::uint32_t channel, Pin direction_pin);

        auto write(Percent output) const -> void;

        auto set_direction_pins(Percent duty_cycle) const -> void;

        auto set_duty_cycle(Percent duty_cycle, Percent max_duty_cycle) const -> void;

        auto change_max_pwm(Percent max_pwm) -> void;

        auto change_inverted(bool inverted) -> void;
    };

} // namespace mrover
