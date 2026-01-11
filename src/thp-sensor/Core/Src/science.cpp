#include "main.h"
#include "thp_sensor.hpp"

extern I2C_HandleTypeDef hi2c1;

THP thp_sensor;

void event_loop() {
	thp_sensor.read_thp();
	HAL_Delay(100);
}

void init() {
	thp_sensor = THP(&hi2c1);
	thp_sensor.init();
	event_loop();
}

extern "C" {
	void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
		thp_sensor.update_thp();
	}

	void HAL_PostInit() {
		init();
	}
}
