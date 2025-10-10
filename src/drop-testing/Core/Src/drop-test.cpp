#include "main.h"
#include "ADXL343.hpp"
#include <queue>
#include <chrono>

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

#define ACCEL_ADDR1 0x1D // Default address when SDO is connected to VCC
#define ACCEL_ADDR2 0x53 // Alternate address when SDO is connected to GND
#define NUM_ADXL    2

namespace mrover {
	ADXL343 accelerometer;
	ADXL343 accel_array[2] = {
			{&hi2c1, ACCEL_ADDR1},
			{&hi2c2, ACCEL_ADDR1}
	};

	struct queueData{
		uint8_t	sensorNumber;
		float timeStamp;
		mrover::AccelData dataVal;
	};

	std::queue<queueData> printQueue;


    void event_loop() {
        while (true) {
        	for(int i = 0; i < NUM_ADXL; i++){
        		//
        		mrover::accel_array[i].read_data(); //read data from accelerometer
        		HAL_Delay(100);
        	}
        	if(!printQueue.empty()){
        		printf("%d, %f, %f %f %f", mrover::printQueue.front().sensorNumber, mrover::printQueue.front().timeStamp,
        				mrover::printQueue.front().dataVal.x, mrover::printQueue.front().dataVal.y, mrover::printQueue.front().dataVal.z);// replace with uart
        		printQueue.pop();
        	}
        }
    }

    void init() {
    	for(int i = 0; i < NUM_ADXL; i++){
    		//mrover::accel_array[i] =
    		while (mrover::accel_array[i].start_accel() != HAL_OK) {} // start measurements on accelerometer
    	}

    	//accelerometer = ADXL343(&hi2c1, ACCEL_ADDR1); //same thing for each bus, alternating addr
    	//while (accelerometer.start_accel() != HAL_OK) {} // start measurements on accelerometer
    	event_loop();
    }
} // namespace mrover

extern "C" {

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
	if(hi2c == &hi2c1){
		//
		float timestamp = 1;
		mrover::accel_array[0].update_accel(); // update the data read from the accelerometer
		mrover::AccelData data = mrover::accel_array[0].getData();
		mrover::printQueue.push({1, timestamp, data});

	} else if (hi2c == &hi2c2){
		//
		float timestamp = 1;
		mrover::accel_array[1].update_accel(); // update the data read from the accelerometer
		mrover::AccelData data = mrover::accel_array[0].getData();
		mrover::printQueue.push({2, timestamp, data});

	}
	//send over uart
	//temp using printf for proof of concept

	//add value to queue

	//where does popping from queue go?

}

void HAL_PostInit() {
    mrover::init();
}

}
