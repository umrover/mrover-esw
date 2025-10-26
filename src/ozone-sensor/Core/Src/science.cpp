#include <cstdint>
#include <cmath>
#include "OzoneSensor.hpp"

extern I2C_HandleTypeDef hi2c1;

namespace mrover {

OzoneSensor ozone_sensor;
double ozone;

void eventLoop() {
	// TODO: implement main loop
	while (true) {
		ozone_sensor.update_ozone();
		HAL_Delay(100);
		ozone_sensor.receive_buf();
		HAL_Delay(100);
	}
}

void init() {
	ozone_sensor = OzoneSensor(&hi2c1);
//	HAL_Delay(180000);
	ozone_sensor.setModes(MEASURE_MODE_PASSIVE);
	HAL_Delay(100);
	eventLoop();
}

}

extern "C" {

	void HAL_PostInit() {
		mrover::init();
	}

	// TODO: implement transmit callback
	void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef* hi2c) {

	}

	// TODO: implement receive callback
	void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		if (hi2c == &hi2c1) {
			mrover::ozone_sensor.calculate_ozone();
			mrover::ozone = mrover::ozone_sensor.get_ozone();
		}
	}

}
