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

void eventLoop() {
	// TODO: implement main loop
	uint8_t command_packet[] = {0xAC, 0x33, 0x00};

	while (true) {
		HAL_Delay(100);

		// Send measure command
		HAL_I2C_Master_Transmit_IT(i2c, TEMP_HUM_ADDRESS << 1, command_packet, 3);
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
		HAL_I2C_Master_Receive_IT(mrover::i2c, TEMP_HUM_ADDRESS << 1 | 0x01, mrover::th_buf, 6);
	}

	// TODO: implement receive callback
	void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		int humidity = mrover::th_buf[1] << 12;
		humidity |= mrover::th_buf[2] << 4;
		humidity |= mrover::th_buf[3] >> 4;

		mrover::humidity = humidity / pow(2.0, 20) * 100.0;

		int temp = (mrover::th_buf[3] & 0x0F) << 16;
		temp |= mrover::th_buf[4] << 8;
		temp |= mrover::th_buf[5];

		mrover::temp = temp / pow(2.0, 20) * 200.0 - 50.0;
	}

}
