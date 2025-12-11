#include <cstdint>

#include <hw/hbridge.hpp>
#include <logger.hpp>
#include <serial/fdcan.hpp>

#include "main.h"
#include "motor.hpp"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_tim.h"

#include "CANBus1.hpp"

#include <functional>

extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

extern I2C_HandleTypeDef hi2c1;

/**
 * For each repeating timer, the update rate is determined by the .ioc file.
 *
 * Specifically the ARR value. You can use the following equation: ARR = (MCU Clock Speed) / (Update Rate) / (Prescaler + 1) - 1
 * For the STM32G4 we have a 140 MHz clock speed configured.
 *
 * You must also set auto reload to true so the interrupt gets called on a cycle.
 */

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
// extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim15;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;


namespace mrover {

    static constexpr uint32_t CAN_ID = 0x05;

    Pin can_tx;
    Pin can_rx;

    CANBus1Handler can_receiver;
    Motor motor;

    auto init() -> void {
        Logger::init(&hlpuart1);
        Logger::get_instance()->info("Initializing...");

        Logger::get_instance()->info("\t...CAN LEDs");
        can_tx = Pin{CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin};
        can_rx = Pin{CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin};

        Logger::get_instance()->info("\t...CAN Transceiver");
        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        can_receiver = CANBus1Handler{FDCAN{&hfdcan1, can_opts}};

        Logger::get_instance()->info("\t...Motor Output");
        motor = Motor{
            HBridge{&htim1, TIM_CHANNEL_1, Pin{MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin}},
        };

        Logger::get_instance()->info("BMC Initialized");
    }

    [[noreturn]] auto loop() -> void {

        size_t n = 0;
        // Logger::get_instance()->info("Delaying for 10s...");
        // HAL_Delay(10000);
        Logger::get_instance()->info("Sending Responses every 5s...");
        for ( ;; ) {
            Logger::get_instance()->info("BMC Main Loop #%u", n);
            can_tx.set();
            can_receiver.send(BMCAck{n}, CAN_ID);
            can_tx.reset();
            HAL_Delay(5000);
            ++n;
        }
    }

    auto receive_can_message() -> void {
        can_rx.set();
        if (auto const recv = can_receiver.receive(CAN_ID); recv) {
            auto const& msg = *recv;
            motor.receive(msg);
        }
        can_rx.reset();
    }

} // namespace mrover

extern "C" {

void PostInit() {
    mrover::init();
}

void Loop() {
    mrover::loop();
}

/**
 * These are interrupt handlers. They are called by the HAL.
 *
 * These are set up in the .ioc file.
 * They have to be enabled in the NVIC settings.
 * It is important th
*/

/**
 * \note Timers have to be started with "HAL_TIM_Base_Start_IT" for this interrupt to work for them.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {

}

/**
 * \note FDCAN1 has to be configured with "HAL_FDCAN_ActivateNotification" and started with "HAL_FDCAN_Start" for this interrupt to work.
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    mrover::receive_can_message();
}

// TODO(eric): Error handling

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef* hfdcan) {}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef* hfdcan, uint32_t ErrorStatusITs) {}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {}
}
