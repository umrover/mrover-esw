#include "OxygenSensor.hpp"

extern I2C_HandleTypeDef hi2c3;

namespace mrover {
    OxygenSensor oxygen_sensor = mrover::OxygenSensor(&hi2c3);
    double oxygen = 0;

    void event_loop() {
        while (true) {
            oxygen = oxygen_sensor.update_oxygen();
            HAL_Delay(50);
        }
    }

    void init() {
        event_loop();
    }

} // namespace mrover

extern "C" {
	void HAL_PostInit() {
		mrover::init();
	}
}
