#include "servo.hpp"

uint32_t CHANNEL;
TIM_HandleTypeDef* TIM_HANDLE;

// TODO: Implement the Servo constructor
Servo::Servo(TIM_HandleTypeDef* tim_handle, uint32_t channel) {
	CHANNEL = channel;
	TIM_HANDLE = tim_handle;
}

// TODO: Start the PWM signal generation
void Servo::start_servo() {
	HAL_TIM_PWM_Start(TIM_HANDLE, CHANNEL);
}

// TODO: Move the servo to the specified angle
void Servo::set_servo_angle(int angle) {
	// Shift angle up to make it positive (0 - 180 range)
	angle += 90;
	double dutyCycle = angle * 0.0002777777778 + 0.05;
	int CCR = dutyCycle * 20000 - 1;
	__HAL_TIM_SET_COMPARE(TIM_HANDLE, CHANNEL, CCR);
}
