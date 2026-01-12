#include <cstdint>
#include <functional>

#include <hw/hbridge.hpp>
#include <hw/ad8418a.hpp>
#include <logger.hpp>
#include <serial/fdcan.hpp>
#include <timer.hpp>

#include "main.h"
#include "config.hpp"
#include "motor.hpp"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_tim.h"

#include "CANBus1.hpp"


extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

extern TIM_HandleTypeDef htim1;
// extern TIM_HandleTypeDef htim2;
// extern TIM_HandleTypeDef htim3;
// extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
// extern TIM_HandleTypeDef htim8;
// extern TIM_HandleTypeDef htim15;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;


namespace mrover {

    static constexpr ADC_HandleTypeDef* ADC = &hadc1;
    static constexpr UART_HandleTypeDef* LPUART = &hlpuart1;
    static constexpr FDCAN_HandleTypeDef* CAN = &hfdcan1;

    static constexpr TIM_HandleTypeDef* MOTOR_PWM_TIM = &htim1;
    static constexpr TIM_HandleTypeDef* TX_TIM = &htim6;  // 10 Hz
    static constexpr TIM_HandleTypeDef* CAN_WWDG_TIM = &htim16;  // 0.1 Hz
    static constexpr TIM_HandleTypeDef* CONTROL_TIM = &htim17;  // 100 Hz

    bmc_config_t config;
    bool initialized = false;

    UART lpuart;
    // NOTE: FDCAN is not here as the CANHandler instance requires ownership of it

    Pin can_tx;
    Pin can_rx;
    CANBus1Handler can_receiver;
    Motor motor;

    Timer tx_tim;
    Timer can_wwdg_tim;
    Timer control_tim;

    /**
     * Send a CAN message defined in CANBus1.dbc on the bus.
     * @param msg CAN message to send
     */
    auto send_can_message(CANBus1Msg_t const& msg) -> void {
        if (!initialized) return;
        can_tx.set();
        can_receiver.send(msg, config.get<bmc_config_t::can_id>());
        Logger::instance().debug("CAN Message Sent");
        can_tx.reset();
    }

    /**
     * Receive and parse a CAN message over the bus.
     * Message should be of a type defined in CANBus1.dbc
     */
    auto receive_can_message() -> void {
        if (!initialized) return;

        // start the CAN watchdog if it lapsed
        if (!can_wwdg_tim.is_enabled()) {
            can_wwdg_tim.start();
        }

        while (can_receiver.get_driver().messages_to_process() > 0) {
            if (auto const recv = can_receiver.receive(config.get<bmc_config_t::can_id>()); recv) {
                can_rx.set();
                auto const& msg = *recv;
                motor.receive(msg);
                can_wwdg_tim.reset();
                can_rx.reset();
            }
        }
    }

    /**
     * Initialization sequence for BMC.
     */
    auto init() -> void {
        // initialize peripherals
        lpuart = UART{LPUART, get_uart_options()};

        // initialize logger
        Logger::init(&lpuart);
        auto const& logger = Logger::instance();
        logger.info("Initializing");

        // setup debug LEDs
        logger.info("...CAN LEDs");
        can_tx = Pin{CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin};
        can_rx = Pin{CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin};

        // setup can transceiver
        logger.info("...CAN Transceiver");
        can_receiver = CANBus1Handler{FDCAN{CAN, get_can_options()}};

        // setup motor instance
        logger.info("...Motor");
        motor = Motor{
            HBridge{MOTOR_PWM_TIM, TIM_CHANNEL_1, Pin{MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin}},
            AD8418A{AnalogPin{ADC, ADC_CHANNEL_0}},
            send_can_message,
            &config,
        };

        // setup timers
        logger.info("...Timers");
        tx_tim = Timer{TX_TIM, true, "TX Timer"};  // transmit timer (on interrupt)
        can_wwdg_tim = Timer{CAN_WWDG_TIM, true, "CAN Watchdog Timer"};  // can watchdog timer (on interrupt)
        control_tim = Timer{CONTROL_TIM, true, "Control Timer"};  // control timer (update driven output, on interrupt)

        // set initialization state and initial error state
        logger.info("BMC Initialized");
        initialized = true;
    }

    /**
     * Callback for timer periods elapsing.
     * Timers have to be started with "HAL_TIM_Base_Start_IT" for this interrupt to work for them.
     * @param htim The timer whose period elapsed
     */
    auto timer_elapsed_callback(TIM_HandleTypeDef const* htim) -> void {
        if (!initialized) return;
        if (htim == TX_TIM) {
            motor.send_state();
        } else if (htim == CAN_WWDG_TIM) {
            can_wwdg_tim.stop();
            motor.tx_watchdog_lapsed();
            Logger::instance().warn("TX Watchdog Lapsed");
        } else if (htim == CONTROL_TIM) {
            motor.drive_output();
        }
    }

    /**
     * Callback enabling asynchronous serial logs via UART/DMA.
     * @param huart UART handle from callback
     */
    auto uart_tx_callback(UART_HandleTypeDef const* huart) -> void {
        if (huart == LPUART) {
            lpuart.handle_tx_complete();
        }
    }

} // namespace mrover

extern "C" {

    void PostInit() {
        mrover::init();
    }

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
        mrover::timer_elapsed_callback(htim);
    }

    void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
        mrover::receive_can_message();
    }

    void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
        mrover::uart_tx_callback(huart);
    }

    // TODO(eric) implement
    void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {}
    void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef* hfdcan) {}
    void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef* hfdcan, uint32_t ErrorStatusITs) {}
    void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {}
    void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {}
    void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {}
}
