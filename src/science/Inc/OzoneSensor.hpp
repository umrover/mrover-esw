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
		SMBus* smbus; // i2c handle pointer
		float ozone; // ozone value in ppm
		uint8_t rx_buf[2]; // receive buffer

	public:
		OzoneSensor() = default;

		OzoneSensor(SMBus* smbus_in)
			: smbus(smbus_in), ozone(0.0) {}

		// returns the current ozone data in ppm
		[[nodiscard]] float get_ozone() const {
			return ozone;
		}

		// updates the value of the sensor
        void update() override {
			uint16_t ozone_raw = ((int16_t)rx_buf[0] << 8) | rx_buf[1];
			ozone = ozone_raw / 1000.0;
		}

        // polls the sensor for data
        void poll() override {
			smbus->async_mem_read(OZONE_ADDR, AUTO_DATA_HIGH_REGISTER, 1, rx_buf);
		}

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
			uint8_t mode = MEASURE_MODE_AUTOMATIC;
			if (!smbus->blocking_mem_write(OZONE_ADDR, MODE_REGISTER, 1, {reinterpret_cast<const char*>(&mode), sizeof(mode)}))
				return false;

			return true;
		}
	};
}
