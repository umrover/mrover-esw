#pragma once

#include "stm32g4xx_hal_tim.h"
#include "AutonLED.hpp"

namespace mrover {
    class PDLB {
    private:
        AutonLED auton_led;
        TIM_HandleTypeDef* blink_tim;

    public:
        PDLB() = default;

        PDLB (AutonLED& auton_led_in, TIM_HandleTypeDef* blink_tim)
            : auton_led(auton_led_in) {}

        void blink() {
            auton_led.blink();
        }

        void set_led (bool red, bool green, bool blue, bool blinking) {
            if (blinking) {
                __HAL_TIM_SET_COUNTER(blink_tim, 0);
                HAL_TIM_Base_Start_IT(blink_tim);
            } else {
                HAL_TIM_Base_Stop_IT(blink_tim);
            }

            auton_led.change_state(red, green, blue, blinking);
        }
    };
} // namespace mrover