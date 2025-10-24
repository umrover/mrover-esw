#include <cstdint>
#include <cmath>
#include "main.h"

// TODO: add sensor I2C address
#define TEMP_HUM_ADDRESS 0x38

extern I2C_HandleTypeDef hi2c3;

namespace mrover {

I2C_HandleTypeDef* i2c;
uint8_t th_buf[6];
double temp;
double humidity;
uint32_t raw_temp;
uint32_t raw_humidity;

void eventLoop() {
	// TODO: implement main loop
	HAL_Delay(100);
	while (true) {
		th_buf[0] = 0xAC;
		th_buf[1] = 0x33;
		th_buf[2] = 0x00;
		HAL_I2C_Master_Transmit_IT(i2c, TEMP_HUM_ADDRESS << 1, th_buf, 3);
		HAL_Delay(1000);
	}

}

void init() {
	i2c = &hi2c3;
	eventLoop();
}

}

extern "C" {

	void HAL_PostInit() {
		mrover::init();
	}

	// TODO: implement transmit callback
	void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef* hi2c) {
		HAL_I2C_Master_Receive_IT(mrover::i2c, (TEMP_HUM_ADDRESS << 1) | 1, mrover::th_buf, 6);

	}

	// TODO: implement receive callback
	void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		mrover::raw_humidity = (mrover::th_buf[1] << 12) + (mrover::th_buf[2] << 4) + (mrover::th_buf[3] >> 4);
		mrover::raw_temp = ((mrover::th_buf[3] & 0x0F) << 16) + (mrover::th_buf[4] << 8) + mrover::th_buf[5];
		mrover::humidity = ((double)mrover::raw_humidity / std::pow(2, 20)) * 100;
		mrover::temp = (((double)mrover::raw_temp / std::pow(2, 20)) * 200) - 50;
	}

}
