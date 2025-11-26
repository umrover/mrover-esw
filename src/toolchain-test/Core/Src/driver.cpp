#include "main.h"

#include <hw/pin.hpp>

#include <memory>

namespace mrover {

    static constexpr bool DEF = true;
    std::unique_ptr<Pin> button;
    std::unique_ptr<Pin> led;
    auto init() -> void {
        button = std::make_unique<Pin>(BTN_GPIO_Port, BTN_Pin);
        led = std::make_unique<Pin>(LED_GPIO_Port, LED_Pin);
    }

    [[noreturn]] auto loop() -> void {

        for ( ;; ) {
            if (DEF) {
                auto const b_state = button->read();
                led->write(b_state);
            } else {
                led->toggle();
                HAL_Delay(500);
            }
        }

    }

} // namespace mrover

extern "C" {

    void PostInit() {
        mrover::init();
    }

    void Loop() {
        mrover::loop();
    }

}
