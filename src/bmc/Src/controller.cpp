#include <cstdint>

#include <hw/hbridge.hpp>
#include <logger.hpp>
#include <serial/fdcan.hpp>

#include "main.h"
#include "config.hpp"
#include "motor.hpp"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_tim.h"

#include "CANBus1.hpp"

#include <functional>

extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern I2C_HandleTypeDef hi2c1;

extern TIM_HandleTypeDef htim1;
// extern TIM_HandleTypeDef htim2;
// extern TIM_HandleTypeDef htim3;
// extern TIM_HandleTypeDef htim4;
// extern TIM_HandleTypeDef htim6;
// extern TIM_HandleTypeDef htim8;
// extern TIM_HandleTypeDef htim15;
// extern TIM_HandleTypeDef htim16;
// extern TIM_HandleTypeDef htim17;


namespace mrover {

    static constexpr uint32_t CAN_ID = 0x05;

    // Hardware Objects
    Pin can_tx;
    Pin can_rx;
    CANBus1Handler can_receiver;
    Motor motor;

    /**
     * Send a CAN message defined in CANBus1.dbc on the bus.
     * @param msg CAN message to send
     */
    auto send_can_message(CANBus1Msg_t const& msg) -> void {
        can_tx.set();
        can_receiver.send(msg, CAN_ID);
        Logger::get_instance()->debug("CAN Message Sent");
        can_tx.reset();
    }

    /**
     * Receive and parse a CAN message over the bus.
     * Message should be of a type defined in CANBus1.dbc
     */
    auto receive_can_message() -> void {
        if (auto const recv = can_receiver.receive(CAN_ID); recv) {
            can_rx.set();
            Logger::get_instance()->debug("CAN Message Received");
            auto const& msg = *recv;
            motor.receive(msg);
            can_rx.reset();
        }
    }

    /**
     * Initialization sequence for BMC.
     */
    auto init() -> void {
        // initialize logger
        Logger::init(&hlpuart1);
        Logger::get_instance()->info("Initializing...");

        // setup debug LEDs
        Logger::get_instance()->info("\t...CAN LEDs");
        can_tx = Pin{CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin};
        can_rx = Pin{CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin};

        // setup can transceiver
        Logger::get_instance()->info("\t...CAN Transceiver");
        can_receiver = CANBus1Handler{FDCAN{&hfdcan1, get_can_options()}};

        // setup motor instance
        Logger::get_instance()->info("\t...Motor");
        motor = Motor{
            HBridge{&htim1, TIM_CHANNEL_1, Pin{MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin}},
            send_can_message,
        };

        // set initialization state and initial error state
        Logger::get_instance()->info("BMC Initialized");
    }

    /**
     * Main execution loop.
     *
     * Spin here, all logic interrupt-driven.
     */
    [[noreturn]] auto loop() -> void {
        for ( ;; ) {
            // periodic delay
            HAL_Delay(10);
        }
    }

} // namespace mrover

extern "C" {

/**
 * Initialization Callback
 */
void PostInit() {
    mrover::init();
}

/**
 * Main Loop Callback
 */
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
