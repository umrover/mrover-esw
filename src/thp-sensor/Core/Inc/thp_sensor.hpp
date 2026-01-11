#include "main.h"

#define BME280_ADDR 0x77
#define BME280_REG_ID 0xD0
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_DATA 0xF7

class THP {
private:
	I2C_HandleTypeDef* i2c;
	uint8_t rx_buffer[8];
	float temp;
	float humidity;
	float pressure;
public:
	THP() = default;

	THP (I2C_HandleTypeDef* i2c_in)
		: i2c(i2c_in) {}

	// initializes the thp sensor
	void init() {
		// set humidity oversampling x1
		uint8_t ctrl_hum = 0x01;
		HAL_I2C_Mem_Write(i2c, BME280_ADDR, BME280_REG_CTRL_HUM, I2C_MEMADD_SIZE_8BIT, &ctrl_hum, 1, HAL_MAX_DELAY);

		// set temp oversampling to x1, pressure x1, and normal mode */
		uint8_t ctrl_meas = 0x27;
		HAL_I2C_Mem_Write(i2c, BME280_ADDR, BME280_REG_CTRL_MEAS, I2C_MEMADD_SIZE_8BIT, &ctrl_meas, 1, HAL_MAX_DELAY);

		// Standby 62.5 ms (close to 10 Hz), filter off
		uint8_t config = 0x20;
		HAL_I2C_Mem_Write(i2c, BME280_ADDR, BME280_REG_CONFIG, I2C_MEMADD_SIZE_8BIT, &config, 1, HAL_MAX_DELAY);
	}

	// reads the data register on the thp sensor
	void read_thp() {
		HAL_I2C_Mem_Read_IT(i2c, BME280_ADDR, BME280_REG_DATA, I2C_MEMADD_SIZE_8BIT, rx_buffer, 8);
	}

	// update the temp, humidity, and pressure variables
	void update_thp() {
	    temp = (rx_buffer[3] << 12) | (rx_buffer[4] << 4) | (rx_buffer[5] >> 4);
	    humidity = (rx_buffer[6] << 8) | rx_buffer[7];
	    pressure = (rx_buffer[0] << 12) | (rx_buffer[1] << 4) | (rx_buffer[2] >> 4);
	}
};
