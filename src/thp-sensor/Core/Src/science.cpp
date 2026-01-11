#include "main.h"
#include "thp_sensor.hpp"

extern I2C_HandleTypeDef hi2c3;

namespace mrover {

THP thp_sensor;

void event_loop() {
	while (true) {
		thp_sensor.read_thp();
		HAL_Delay(100);
	}
}

void init() {
	thp_sensor = THP(&hi2c3);
	thp_sensor.init();
	event_loop();
}
}

extern "C" {
	void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
		mrover::thp_sensor.update_thp();
	}

	void HAL_PostInit() {
		mrover::init();
	}
}
