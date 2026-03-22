#include "ScienceSensor.hpp"
#include <serial/smbus.hpp>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"

#define CO2_ADDR 0x29

namespace mrover {
	enum Mode {
		TX = 0,
		RX = 1,
	};

	class CO2Sensor : public ScienceSensor{
	private:
		SMBus* smbus;
		uint8_t req_buf[2];
        uint8_t rx_buf[2];
		float percent; // ozone value in ppm
		Mode mode;

		// requests raw co2 data over i2c, when sensor responds with data callback will be hit and will call receive_buf
		void request_co2() {
			smbus->async_transmit(CO2_ADDR, {reinterpret_cast<const char*>(req_buf), sizeof(req_buf)});
		}

		// receives raw ozone data over i2c
		void receive_buf() {
			// on the first read sensor needs 100ms to update register
			static bool first_read = true;
			if (first_read) {
				HAL_Delay(100);
				first_read = false;
			}

			smbus->async_receive(CO2_ADDR, rx_buf);
		}

	public:
		CO2Sensor() = default;

		explicit CO2Sensor (SMBus* smbus_in)
			: smbus(smbus_in), req_buf{0x36, 0x39}, rx_buf{0x00, 0x00}, percent(0.0), mode(Mode::TX) {}

		// returns the current ozone data in ppm
		[[nodiscard]] float get_co2() const {
			return percent;
		}

		// updates the value of the sensor
        void update() override {
			uint16_t raw = (rx_buf[0] << 8) | rx_buf[1];
            percent = (float(raw - (1 << 14)) / (1 << 15)) * 100.0f;
		}

        // polls the sensor for data
        void poll() override {
			if (mode == Mode::TX) {
				request_co2();
				mode = Mode::RX;
			} else if (mode == Mode::RX) {
				receive_buf();
				mode = Mode::TX;
			}
		}

        // attempts to initialize sensor, returns true on success and false on failure
        bool init() override {
			// Disable CRC (0x3768)
			uint8_t tx_buf1[2] = {0x37, 0x68};
            if (!smbus->blocking_transmit(CO2_ADDR, {reinterpret_cast<const char*>(tx_buf1), sizeof(tx_buf1)}))
				return false;

            // Set measurement mode -> standard measurement mode with 0-25% concentration in air
			uint8_t tx_buf2[4] = {0x36, 0x15, 0x00, 0x11};
            if (!smbus->blocking_transmit(CO2_ADDR, {reinterpret_cast<const char*>(tx_buf2), sizeof(tx_buf2)}))
				return false;

			mode = Mode::TX;

			return true;
		}
	};
}