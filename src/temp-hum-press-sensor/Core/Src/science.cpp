#include <tempHumPressure.hpp>
#include <cstdint>
#include <cmath>

extern I2C_HandleTypeDef hi2c1;

namespace mrover {

TempHumPressureSensor tempHumPress_sensor;
double ozone;

void eventLoop() {
	while (true) {

		HAL_Delay(200);
	}
}

void init() {
	tempHumPress_sensor = TempHumPressureSensor(&hi2c1);
	tempHumPress_sensor.set_settings();
	tempHumPress_sensor.set_mode();
	eventLoop();
}

}

extern "C" {

	void HAL_PostInit() {
		mrover::init();
	}


	void HAL_I2C_MemTxCpltCallback (I2C_HandleTypeDef* hi2c) {
		if (hi2c == &hi2c1) {

		}

	}


	void HAL_I2C_MemRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		if (hi2c == &hi2c1) {

		}
	}

}
