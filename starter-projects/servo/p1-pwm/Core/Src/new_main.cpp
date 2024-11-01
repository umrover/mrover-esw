#include "new_main.h"
#include "servo.hpp"

extern TIM_HandleTypeDef htim1;

void new_main(){

	Servo servo(&htim1,TIM_CHANNEL_1);
	servo.start_servo();


  while(1){
	  servo.set_servo_angle(0);
	  HAL_Delay(1000);
	  servo.set_servo_angle(90);
	  HAL_Delay(1000);
	  servo.set_servo_angle(0);
	  HAL_Delay(1000);
	  servo.set_servo_angle(180);
	  	  HAL_Delay(1000);

  }
}
