#include "main.h"

#define CO2_ADDR 0x29

namespace mrover {
	class CO2Sensor {
	private:
		I2C_HandleTypeDef* i2c; // i2c handle pointer
        uint8_t rx_buf[2];
		double ppm; // ozone value in ppm

	public:
		CO2Sensor() = default;

		CO2Sensor (I2C_HandleTypeDef* i2c_in)
			: i2c(i2c_in), ppm(0.0) {}

		// returns the current ozone data in ppm
		double get_co2() {
			return ppm;
		}

        // updates ppm using rx_buf data -> conversion formula: ppm = ((rx_buf - 2^14) / 2^15) * 100
        void update_co2() {
            uint16_t raw = (rx_buf[0] << 8) | rx_buf[1];
            ppm = ((raw - (1 << 14)) / (1 << 15)) * 100.00;
        }

        // requests raw co2 data over i2c, when sensor responds with data callback will be hit and will call receive_buf
		void request_co2() {
            uint8_t tx_buf[2] = {0x36, 0x39};
			HAL_I2C_Master_Transmit_IT(i2c, CO2_ADDR << 1, tx_buf, 2);
		}

		// receives raw ozone data over i2c
		void receive_buf() {
			HAL_I2C_Master_Receive_IT(i2c, (CO2_ADDR << 1) | 1, rx_buf, 2);
		}

		// initializes the sensor to be in AUTO mode (sensor constantly sends data)
		void init() {
            // Disable CRC (0x3768)
			uint8_t tx_buf1[2] = {0x37, 0x68};
            HAL_I2C_Master_Transmit(i2c, CO2_ADDR << 1, tx_buf1, 2, HAL_MAX_DELAY);

            // Set measurement mode -> standard measurement mode with 0-100% concentration in air
			uint8_t tx_buf2[4] = {0x36, 0x15, 0x00, 0x13};
            HAL_I2C_Master_Transmit(i2c, CO2_ADDR << 1, tx_buf2, 4, HAL_MAX_DELAY);
		}
	};
}