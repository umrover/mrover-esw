#include "servo.hpp"

// TODO: Implement the Servo constructor
Servo::Servo(TIM_HandleTypeDef* tim_handle, uint32_t channel) : m_tim_handle(tim_handle), m_channel(channel) {}

// TODO: Start the PWM signal generation
void Servo::start_servo() {
	HAL_TIM_PWM_Start(m_tim_handle, m_channel);
}

// TODO: Move the servo to the specified angle
void Servo::set_servo_angle(int angle) {
	const int PERIODS_PER_MS = 40000 / 20;
	int pulse_length = PERIODS_PER_MS + PERIODS_PER_MS * (angle + 90) / 180;
	__HAL_TIM_SET_COMPARE(m_tim_handle, m_channel, pulse_length - 1);
}
