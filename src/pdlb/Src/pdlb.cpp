#include "main.h"
#include "stm32g4xx_hal_tim.h"
#include "PDLB.hpp"

extern TIM_HandleTypeDef htim2;

namespace mrover {
    static constexpr TIM_HandleTypeDef* BLINK_TIM = &htim2; // 500ms

    PDLB pdlb;

    void event_loop() {
        while (true) {
            // test red
            pdlb.set_led(true, false, false, false);
            HAL_Delay(1000);

            // test blinking green
            pdlb.set_led(false, true, false, true);
            HAL_Delay(1000);

            // test blue
            pdlb.set_led(false, false, true, false);
            HAL_Delay(1000);
        }
    }

    void init() {
        auto auton_led = AutonLED{
            Pin(AUTON_LED_R_GPIO_Port, AUTON_LED_R_Pin),
            Pin(AUTON_LED_G_GPIO_Port, AUTON_LED_G_Pin),
            Pin(AUTON_LED_B_GPIO_Port, AUTON_LED_B_Pin)};

        pdlb = PDLB{auton_led, BLINK_TIM};
    }
} // namespace mrover

extern "C" {
    void PostInit() {
        mrover::init();
    }

    void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
        if (htim == mrover::BLINK_TIM) {
            mrover::pdlb.blink();
        }
    }
}
