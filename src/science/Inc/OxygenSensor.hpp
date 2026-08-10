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
        SMBus* m_smbus;
        float m_calibration_multiplier;
        float m_percent;
        uint8_t m_rx_buf[3];

    public:
        OxygenSensor() = default;

        OxygenSensor(SMBus* smbus_in)
            : m_smbus(smbus_in), m_calibration_multiplier(0.0), m_percent(0.0) {};

        // returns current percentage of oxygen
        [[nodiscard]] float get_oxygen() const {
            return m_percent;
        }

        // updates the value of the sensor
        void update() override {
            m_percent = (m_calibration_multiplier * (((float) m_rx_buf[0]) + ((float) m_rx_buf[1] / 10.0) + ((float) m_rx_buf[2] / 100.0)));
        }

        // polls the sensor for data
        void poll() override {
            m_smbus->async_mem_read(DEV_ADDR, OXYGEN_DATA_REGISTER, 1, m_rx_buf);
        }

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
            uint8_t calibration_buf[1];
            if (!m_smbus->blocking_mem_read(DEV_ADDR, OXYGEN_KEY_REGISTER, 1, calibration_buf))
                return false;
            
            m_calibration_multiplier = calibration_buf[0] / 1000.0;
            if (m_calibration_multiplier == 0)
                m_calibration_multiplier = 20.9 / 120.0;

            return true;
        }
    }; // class UVSensor
} // namespace mrover
