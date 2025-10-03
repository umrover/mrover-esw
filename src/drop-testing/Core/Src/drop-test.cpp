#include "main.h"
#include "ADXL343.hpp"
#include <queue>

extern I2C_HandleTypeDef hi2c1;

#define ACCEL_ADDR1 0x1D // Default address when SDO is connected to VCC
#define ACCEL_ADDR2 0x53 // Alternate address when SDO is connected to GND

namespace mrover {
	ADXL343 accelerometer;

	struct queueData{
		uint8_t	sensorNumber;
		float timeStamp;
		AccelData dataVal;
	};

	std::queue<queueData> printQueue;


    void event_loop() {
        while (true) {
        	accelerometer.read_data(); // read data from accelerometer
        	//would do the same thing for each of the accelerometers
        	HAL_Delay(100);
        }
    }

    void init() {
    	accelerometer = ADXL343(&hi2c1, ACCEL_ADDR1); //same thing for each bus, alternating addr
    	while (accelerometer.start_accel() != HAL_OK) {} // start measurements on accelerometer
    	event_loop();
    }
} // namespace mrover

extern "C" {

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
	mrover::accelerometer.update_accel(); // update the data read from the accelerometer
	//send over uart
	//temp using printf for proof of concept
	mrover::AccelData data = mrover::accelerometer.getData();
	//add value to queue

	//where does popping from queue go?

}

void HAL_PostInit() {
    mrover::init();
}

}
