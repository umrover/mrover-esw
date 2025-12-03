#include <cstdint>
#include <cmath>
#include "main.h"

// TODO: add sensor I2C address
#define TEMP_HUM_ADDRESS 0111000

extern I2C_HandleTypeDef hi2c3;

namespace mrover {
	I2C_HandleTypeDef* i2c;
	uint8_t th_buf[6];
	double temp;
	double humidity;

	void eventLoop() {
		// TODO: implement main loop
		while (true) {
			HAL_Delay(100);
			uint8_t command[3] = {0xAC, 0x33, 0x00};
			HAL_I2C_Master_Transmit_IT(i2c, (TEMP_HUM_ADDRESS << 1) | 0x00, command, 3);
		}
	}

	void init() {
		HAL_Delay(150);
		i2c = &hi2c3;
		HAL_Delay(10);
		eventLoop();
	}
}

extern "C" {
	void HAL_PostInit() {
		mrover::init();
	}

	// TODO: implement transmit callback
	void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef* hi2c) {
		HAL_I2C_Master_Receive_IT(hi2c, TEMP_HUM_ADDRESS, mrover::th_buf, 6);
	}

	// TODO: implement receive callback
	void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		// Store all of the Humidity data in 32 bits
		uint32_t humidityData = (mrover::th_buf[1] << 12) + (mrover::th_buf[2] << 4) + (mrover::th_buf[3] >> 4);
		// Store all of the temp data in 32 bits
		uint32_t tempData = ((mrover::th_buf[3] & 0x0F) << 16) + (mrover::th_buf[4] << 8) + (mrover::th_buf[5]);
		
		// Humidity conversion
		mrover::humidity = (humidityData / pow(2,20)) * 100;
		// Temperature conversion
		mrover::temp = ((tempData / pow(2,20)) * 200) - 50;
	}
}
