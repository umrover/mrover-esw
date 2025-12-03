#include "new_main.h"
#include "servo.hpp"

extern TIM_HandleTypeDef htim1;

void new_main(){

	TIM_HandleTypeDef* TIMptr = &htim1;
	uint32_t chan = TIM_CHANNEL_1;
	Servo S(TIMptr, chan);

	S.start_servo();

  while(1){
	  S.set_servo_angle(-90);
	  HAL_Delay(3000);

	  S.set_servo_angle(0);
	  HAL_Delay(3000);

	  S.set_servo_angle(90);
	  HAL_Delay(3000);

	  S.set_servo_angle(0);
	  HAL_Delay(3000);
  }
}
