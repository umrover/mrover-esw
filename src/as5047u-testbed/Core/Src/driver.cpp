#include "main.h"
#include <cstdio>

extern "C" {
    // reference the HAL SPI handle defined in main.c
    #include "as5047u.h"
    extern SPI_HandleTypeDef hspi3;
}

namespace mrover {

    // TODO instantiate your class here to test
    static AS5047U_HandleTypeDef s_enc;
    static bool s_initialized = false;

    auto init() -> void {
        // TODO initialization logic here
        AS5047U_Init(&s_enc, &hspi3, CS_GPIO_Port, CS_Pin);

        s_initialized = true;
    }

    [[noreturn]] auto loop() -> void {

        for ( ;; ) {
            // TODO infinite loop logic here
            float angle_deg = AS5047U_ReadAngle(&s_enc);
            float vel_dps   = AS5047U_ReadVelocity(&s_enc);

            printf("Angle: %.2f deg, Velocity: %.2f deg/s\r\n", angle_deg, vel_dps);
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
