#include "main.h"
#include "ADXL343.hpp"

extern I2C_HandleTypeDef hi2c1;

#define ACCEL_ADDR1 0x1D // Default address when SDO is connected to VCC
#define ACCEL_ADDR2 0x53 // Alternate address when SDO is connected to GND

namespace mrover {
	ADXL343 accelerometer;

    void event_loop() {
        while (true) {
        	accelerometer.read_data(); // read data from accelerometer
        	HAL_Delay(100);
        }
    }

    void init() {
    	accelerometer = ADXL343(&hi2c1, ACCEL_ADDR1);
    	while (accelerometer.start_accel() != HAL_OK) {} // start measurements on accelerometer
    	event_loop();
    }
} // namespace mrover

extern "C" {

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
	mrover::accelerometer.update_accel(); // update the data read from the accelerometer
}

void HAL_PostInit() {
    mrover::init();
}

}
