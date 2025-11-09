#include "main.h"
#include <cstdio>
#include "as5047u.hpp"

extern "C" {
    // reference the HAL SPI handle defined in main.c
    // #include "as5047u.h"
    extern SPI_HandleTypeDef hspi3;
}

namespace mrover {

    // TODO instantiate your class here to test
    //static AS5047U_HandleTypeDef s_enc;
    static AS5047U s_enc(&hspi3, {CS_GPIO_Port, CS_Pin});
    static bool s_initialized = false;

    auto init() -> void {
        // TODO initialization logic here
        // AS5047U_Init(&s_enc, &hspi3, CS_GPIO_Port, CS_Pin);
        s_enc.init();
        s_initialized = true;
    }

    [[noreturn]] auto loop() -> void {

        for ( ;; ) {
            // TODO infinite loop logic here
            s_enc.update_position();
            s_enc.update_velocity();

            const uint16_t raw = s_enc.get_position();         // 0..16383
            const float deg = static_cast<float>(raw) * 360.0f / 16384.0f;
            // const float  deg = raw * 360.0f / 16384.0f;    
            const float  dps = s_enc.get_velocity();         

            std::printf("Angle: %.2f deg, Velocity: %.2f deg/s\r\n", deg, dps);
            HAL_Delay(1000);
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
