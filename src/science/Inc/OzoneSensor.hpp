#include "ScienceSensor.hpp"
#include <serial/smbus.hpp>
#include "stm32g4xx_hal_def.h"
#include <cstdint>

#define OZONE_ADDR	0x73 // i2c addr
#define MEASURE_MODE_AUTOMATIC	0x00 // auto mode
#define AUTO_DATA_HIGH_REGISTER	0x09 // auto mode data high byte
#define AUTO_DATA_LOW_REGISTER	0x0A // auto mode data low byte
#define MODE_REGISTER	0x03 // mode register

namespace mrover {
	class OzoneSensor : public ScienceSensor{
	private:
		SMBus* m_smbus; // i2c handle pointer
		float m_ozone; // ozone value in ppm
		uint8_t m_rx_buf[2]; // receive buffer

	public:
		OzoneSensor() = default;

		OzoneSensor(SMBus* smbus_in)
			: m_smbus(smbus_in), m_ozone(0.0) {}

		// returns the current m_ data in ppm
		[[nodiscard]] float get_ozone() const {
			return m_ozone;
		}

		// updates the value of the sensor
        void update() override {
			uint16_t ozone_raw = ((int16_t)m_rx_buf[0] << 8) | m_rx_buf[1];
			m_ozone = ozone_raw / 1000.0;
		}

        // polls the sensor for data
        void poll() override {
			m_smbus->async_mem_read(OZONE_ADDR, AUTO_DATA_HIGH_REGISTER, 1, m_rx_buf);
		}

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
			uint8_t mode = MEASURE_MODE_AUTOMATIC;
			if (!m_smbus->blocking_mem_write(OZONE_ADDR, MODE_REGISTER, 1, {reinterpret_cast<const char*>(&mode), sizeof(mode)}))
				return false;

			return true;
		}
	};
}
