#include "hw/hbridge.hpp"


namespace mrover {

    HBridge::HBridge(TIM_HandleTypeDef* timer, std::uint32_t channel, Pin direction_pin)
        : m_direction_pin{direction_pin},
          m_timer{timer},
          m_channel{channel},
          m_max_pwm{0_percent} {

        // Prevent the motor from spinning on boot up
        __HAL_TIM_SET_COMPARE(m_timer, m_channel, 0);
        check(HAL_TIM_PWM_Start(m_timer, m_channel) == HAL_OK, Error_Handler);
    }


    auto HBridge::write(Percent output) const -> void {
        // Set direction pins/duty cycle
        set_direction_pins(output);
        set_duty_cycle(output, m_max_pwm);
    }

    auto HBridge::set_direction_pins(Percent duty_cycle) const -> void {
        GPIO_PinState pin_state;
        if (!m_is_inverted) {
            pin_state = (duty_cycle > 0_percent) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        } else {
            pin_state = (duty_cycle > 0_percent) ? GPIO_PIN_RESET : GPIO_PIN_SET;
        }

        m_direction_pin.write(pin_state);
    }

    auto HBridge::set_duty_cycle(Percent duty_cycle, Percent max_duty_cycle) const -> void {
        // Clamp absolute value of the duty cycle to the supported range
        duty_cycle = std::clamp(abs(duty_cycle), 0_percent, max_duty_cycle);

        // Set CCR register
        // The CCR register compares its value to the timer and outputs a signal based on the result
        // The ARR register sets the limit for when the timer register resets to 0.
        auto limit = __HAL_TIM_GetAutoreload(m_timer);
        __HAL_TIM_SetCompare(m_timer, m_channel, static_cast<std::uint32_t>(std::round(duty_cycle.get() * limit)));
        // TODO(eric) we should error if the registers are null pointers
    }

    auto HBridge::change_max_pwm(Percent max_pwm) -> void {
        m_max_pwm = max_pwm;
    }

    auto HBridge::change_inverted(bool inverted) -> void {
        m_is_inverted = inverted;
    }

} // namespace mrover
