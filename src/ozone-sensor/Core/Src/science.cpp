#include <OzoneSensor.hpp>
#include <cstdint>
#include <cmath>

extern I2C_HandleTypeDef hi2c1;

namespace mrover {

OzoneSensor ozone_sensor;
double ozone;

void eventLoop() {
	while (true) {
		ozone_sensor.update_ozone1();
		HAL_Delay(200);
	}
}

void init() {
	ozone_sensor = OzoneSensor(&hi2c1);
	ozone_sensor.setPassive();
	//needs 3 minutes to warm up
	eventLoop();
}

}

extern "C" {

	void HAL_PostInit() {
		mrover::init();
	}


	void HAL_I2C_MemTxCpltCallback (I2C_HandleTypeDef* hi2c) {
		if (hi2c == &hi2c1) {
			if(mrover::ozone_sensor.read == true){
				mrover::ozone_sensor.receive_buf();
				mrover::ozone_sensor.read = false;
			}
		}

	}


	void HAL_I2C_MemRxCpltCallback (I2C_HandleTypeDef* hi2c) {
		if (hi2c == &hi2c1) {
			mrover::ozone_sensor.calculate_ozone();
			mrover::ozone = mrover::ozone_sensor.get_ozone();
		}
	}

}
