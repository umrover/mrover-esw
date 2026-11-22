#include <hw/pin.hpp>
#include <rtos/semaphore.hpp>
#include <rtos/task.hpp>

#include "main.h"

namespace mrover {

    static constexpr uint32_t BLINK_PERIOD_MS = 500;
    static constexpr uint32_t DEBOUNCE_MS = 50;

    static Pin led{LED_GPIO_Port, LED_Pin};

    static Semaphore button_pressed{};

    static void blink_task();
    static void button_task();

    static Task<512> blink{"blink", blink_task, osPriorityAboveNormal};
    static Task<512> button{"button", button_task, osPriorityLow};

    static void blink_task() {
        for (;;) {
            led.toggle();
            osDelay(BLINK_PERIOD_MS);
        }
    }

    static void button_task() {
        bool blinking = true;

        for (;;) {
            if (!button_pressed.acquire()) continue;

            osDelay(DEBOUNCE_MS);
            while (button_pressed.try_acquire()) {}

            blinking = !blinking;
            if (blinking) {
                blink.resume();
            } else {
                blink.suspend();
                led.reset();
            }
        }
    }

} // namespace mrover

extern "C" {

void PostInit() {
    osKernelInitialize();
    mrover::blink.start();
    mrover::button.start();
    osKernelStart();
}

void HAL_GPIO_EXTI_Callback(uint16_t const pin) {
    if (pin == BUTTON_Pin) {
        mrover::button_pressed.release();
    }
}

} // extern "C"
