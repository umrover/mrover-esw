// https://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf

#pragma once

#include <cstdint>

#include "i2c_bus.hpp"

namespace accel {

    struct Vec3 {
        double x = 0, y = 0, z = 0;
    };

    struct ImuSample {
        long long time; // ms since epoch
        Vec3 accel;     // g
        Vec3 gyro;      // deg/s
    };

    /**
     * @brief Polling-only MPU-6050 driver
     *
     * Configured for ±2 g / ±250 dps, 1 kHz internal sample rate, ~44 Hz DLPF.
     */
    class MPU6050 {
    private:
        I2CBus& bus;
        uint8_t addr;

    public:
        // AD0 low -> 0x68, AD0 high -> 0x69
        static constexpr uint8_t ADDR_AD0_LOW = 0x68;
        static constexpr uint8_t ADDR_AD0_HIGH = 0x69;

        explicit MPU6050(I2CBus& bus, uint8_t addr = ADDR_AD0_LOW);

        /**
         * @brief Check WHO_AM_I, wake the device, and configure ranges
         *
         * @return false if the device didn't respond or a write failed
         */
        auto init() -> bool;

        /**
         * @brief Burst read accel + gyro from the same sample
         *
         * @return false on I2C failure
         */
        auto read(ImuSample& out) -> bool;
    };

} // namespace accel
