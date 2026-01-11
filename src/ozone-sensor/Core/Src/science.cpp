#include <OzoneSensor.hpp>
#include <cstdint>
#include <cmath>

extern I2C_HandleTypeDef hi2c3;

namespace mrover {
OzoneSensor ozone_sensor;
double ozone;

void eventLoop() {
	while (true) {
		mrover::ozone_sensor.receive_buf();
		HAL_Delay(200);
	}
}

void init() {
	ozone_sensor = OzoneSensor(&hi2c3);
	ozone_sensor.init();
	eventLoop();
}
}

extern "C" {
	void HAL_PostInit() {
		mrover::init();
	}

	void HAL_I2C_MemRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		if (hi2c == &hi2c3) {
			mrover::ozone_sensor.update_ozone();
		}
	}
}
