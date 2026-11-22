#include "main.h"
#include "servo.hpp"

extern TIM_HandleTypeDef htim1;
extern FDCAN_HandleTypeDef hfdcan1;

namespace mrover {

    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t rx_buf[8];

    Servo servo(&htim1, TIM_CHANNEL_1, 180, 1000, 2000);

    auto init() -> void {
        servo.start_servo();
        HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
        HAL_FDCAN_Start(&hfdcan1);
    }

    auto recv(FDCAN_HandleTypeDef* hcan) -> void {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        if (HAL_FDCAN_GetRxMessage(hcan, FDCAN_RX_FIFO0, &rx_header, rx_buf) != HAL_OK) {
            Error_Handler();
        }
        servo.set_angle(rx_buf[0]);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    }


} // namespace mrover


extern "C" {

void Init() {
    mrover::init();
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    mrover::recv(hfdcan);
}
}
