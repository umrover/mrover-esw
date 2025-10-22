#include "main.h"
#include "ADXL343.hpp"
#include <queue>
#include <chrono>

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;

#define ACCEL_ADDR1 0x1D // Default address when SDO is connected to VCC
#define ACCEL_ADDR2 0x53 // Alternate address when SDO is connected to GND
#define NUM_ADXL 4

namespace mrover {
	ADXL343 accel_array[NUM_ADXL] = {
			{&hi2c1, ACCEL_ADDR1, 1},
			{&hi2c1, ACCEL_ADDR2, 2},
			{&hi2c2, ACCEL_ADDR1, 3},
			{&hi2c3, ACCEL_ADDR1, 4},
	};

//	struct queueData{
//		uint8_t	sensorNumber;
//		float timeStamp;
//		mrover::AccelData dataVal;
//	};
//
//	std::queue<queueData> printQueue;


    void event_loop() {
        while (true) {
        	for(int i = 0; i < NUM_ADXL; i++){
        		mrover::accel_array[i].read_data(); //read data from accelerometer
        		HAL_Delay(10);
        	}
//        	if(!printQueue.empty()){
//        		printf("%d, %f, %f %f %f", mrover::printQueue.front().sensorNumber, mrover::printQueue.front().timeStamp,
//        				mrover::printQueue.front().dataVal.x, mrover::printQueue.front().dataVal.y, mrover::printQueue.front().dataVal.z);// replace with uart
//        		printQueue.pop();
//        	}
        }
    }

    void init() {
    	for(int i = 0; i < NUM_ADXL; i++){
    		while (mrover::accel_array[i].start_accel() != HAL_OK) {
    			printf("failed to start accelerometer %d\n", (i + 1));
    			HAL_Delay(100);
    		} // start measurements on accelerometer
    		printf("successfully started accelerometer %d\n", (i + 1));
    	}

    	//accelerometer = ADXL343(&hi2c1, ACCEL_ADDR1); //same thing for each bus, alternating addr
    	//while (accelerometer.start_accel() != HAL_OK) {} // start measurements on accelerometer
    	event_loop();
    }
} // namespace mrover

extern "C" {

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
	uint32_t sensor_id = mrover::current_sensor;
	uint32_t timestamp = HAL_GetTick();
	mrover::AccelData data;

	mrover::accel_array[sensor_id - 1].update_accel(); // update the data read from the accelerometer
	data = mrover::accel_array[sensor_id - 1].getData();

	printf("%u %u %f %f %f\n", sensor_id, timestamp, data.x, data.y, data.z);
}

void HAL_PostInit() {
    mrover::init();
}

}
