#include "ScienceSensor.hpp"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_i2c.h"

#define DEV_ADDR 0x70
#define OXYGEN_DATA_REGISTER 0x03
#define OXYGEN_KEY_REGISTER 0x0A

namespace mrover {
    class OxygenSensor : public ScienceSensor{
    private:
        I2C_HandleTypeDef* i2c;
        float calibration_multiplier;
        float percent;
        uint8_t rx_buf[3];

    public:
        OxygenSensor() = default;

        OxygenSensor(I2C_HandleTypeDef* i2c_in)
            : i2c(i2c_in), calibration_multiplier(0.0), percent(0.0) {};

        // returns current percentage of oxygen
        [[nodiscard]] float get_oxygen() const {
            return percent;
        }

        // updates the value of the sensor
        void update() override {
            percent = (calibration_multiplier * (((float) rx_buf[0]) + ((float) rx_buf[1] / 10.0) + ((float) rx_buf[2] / 100.0)));
        }

        // polls the sensor for data
        void poll() override {
            HAL_I2C_Mem_Read_IT(i2c, (DEV_ADDR << 1) | 1, OXYGEN_DATA_REGISTER, 1, rx_buf, 3);
        }

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
            uint8_t calibration_buf[1];
            if (HAL_I2C_Mem_Read(i2c, (DEV_ADDR << 1) | 1, OXYGEN_KEY_REGISTER, 1, calibration_buf, 2, HAL_MAX_DELAY) != HAL_OK)
                return false;
            
            calibration_multiplier = calibration_buf[0] / 1000.0;
            if (calibration_multiplier == 0)
                calibration_multiplier = 20.9 / 120.0;

            return true;
        }
    }; // class UVSensor
} // namespace mrover
