#include <hw/pin.hpp>

#include "main.h"


namespace mrover {

    Pin led, button;

    auto init() -> void {
        led = Pin{GPIOA, GPIO_PIN_5};
        button = Pin{GPIOC, GPIO_PIN_13};
    }

    [[noreturn]] auto loop() -> void {
        for ( ;; ) {
            const auto state = button.read();
            led.write(state == GPIO_PIN_RESET ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
    }

} // namespace mrover


extern "C" {

void HAL_PostInit() {
    mrover::init();
}

void HAL_Loop() {
    mrover::loop();
}

}
