#include "ScienceSensor.hpp"
#include <serial/smbus.hpp>
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_i2c.h"

#define DEV_ADDR 0x70
#define OXYGEN_DATA_REGISTER 0x03
#define OXYGEN_KEY_REGISTER 0x0A

namespace mrover {
    class OxygenSensor : public ScienceSensor{
    private:
        SMBus* smbus;
        float calibration_multiplier;
        float percent;
        uint8_t rx_buf[3];

    public:
        OxygenSensor() = default;

        OxygenSensor(SMBus* smbus_in)
            : smbus(smbus_in), calibration_multiplier(0.0), percent(0.0) {};

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
            smbus->async_mem_read(DEV_ADDR, OXYGEN_DATA_REGISTER, 1, rx_buf);
        }

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
            uint8_t calibration_buf[1];
            if (!smbus->blocking_mem_read(DEV_ADDR, OXYGEN_KEY_REGISTER, 1, calibration_buf))
                return false;
            
            calibration_multiplier = calibration_buf[0] / 1000.0;
            if (calibration_multiplier == 0)
                calibration_multiplier = 20.9 / 120.0;

            return true;
        }
    }; // class UVSensor
} // namespace mrover
