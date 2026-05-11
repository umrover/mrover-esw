#include <MRoverCAN.hpp>
#include <hw/limit_switch.hpp>
#include <hw/pin.hpp>
#include <logger.hpp>
#include <serial/fdcan.hpp>
#include <sys.hpp>
#include <timer.hpp>

#include "lim_config.hpp"
#include "main.h"
#include "limits.hpp"
#include "type.hpp"


extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;

extern TIM_HandleTypeDef htim6;


namespace mrover {

    static constexpr UART_HandleTypeDef* LPUART_1 = &hlpuart1;
    static constexpr FDCAN_HandleTypeDef* FDCAN_1 = &hfdcan1;

    static constexpr TIM_HandleTypeDef* TX_TIM = &htim6; // 10 Hz

    lim_config_t config;
    bool volatile initialized = false;
    bool volatile tx_pending = false;

    // Peripherals
    std::optional<UART> lpuart;
    std::optional<FDCAN> fdcan;

    // Timers
    std::optional<Timer> tx_tim;

    // Hardware Units
    std::optional<Pin> pgood;
    std::optional<Pin> can_tx;
    std::optional<Pin> can_rx;
    std::optional<MRoverCANHandler> can_receiver;
    std::optional<LimitHandler> limit_handler;

    /**
     * Send a CAN message defined in MRoverCAN.dbc on the bus.
     * @param msg CAN message to send
     */
    auto send_can_message(MRoverCANMsg_t const& msg) -> void {
        if (!initialized) return;
        static std::optional<uint8_t> can_id = std::nullopt;
        static std::optional<uint8_t> host_can_id = std::nullopt;

        if (!can_id.has_value()) can_id = config.get<lim_config_t::can_id>();
        if (!host_can_id.has_value()) host_can_id = config.get<lim_config_t::host_can_id>();

        can_tx->set();
        can_receiver->send(msg, can_id.value(), host_can_id.value());
        can_tx->reset();
    }

    /**
     * Receive and parse a CAN message over the bus.
     * Message should be of a type defined in MRoverCAN.dbc
     */
    auto receive_can_message() -> void {
        if (!initialized) return;

        while (fdcan->messages_to_process() > 0) {
            if (auto const recv = can_receiver->receive(); recv) {
                can_rx->set();
                auto const& msg = *recv;
                limit_handler->receive(msg);
                can_rx->reset();
            }
        }
    }

    /**
     * Initialization sequence for BMC.
     */
    auto init() -> void {
        System::InterruptGuard guard{};
        System::get().init(get_sys_options());

        // pgood
        pgood.emplace(PGOOD_GPIO_Port, PGOOD_Pin);
        pgood->set();

        // initialize peripherals
        lpuart.emplace(LPUART_1, get_uart_options());
        fdcan.emplace(FDCAN_1, get_can_options(&config));

        // initialize logger
        Logger::init(&*lpuart);

        // setup timers
        tx_tim.emplace(TX_TIM, true); // transmit timer (on interrupt)

        // setup debug LEDs
        can_tx.emplace(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin);
        can_rx.emplace(CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin);

        // initialize fdcan
        can_receiver = MRoverCANHandler{&*fdcan};

        // setup motor instance
        limit_handler.emplace(
                LimitSwitch{Pin{LIMIT_A_GPIO_Port, LIMIT_A_Pin}},
                LimitSwitch{Pin{LIMIT_B_GPIO_Port, LIMIT_B_Pin}},
                send_can_message,
                &config);

        // set initialization state and initial error state
        initialized = true;
        Logger::instance().info("initialized!");
    }

    [[noreturn]] auto loop() -> void {
        for (;;) {
            // TODO(eric) feels like FreeRTOS would be nice here
            if (tx_pending) {
                limit_handler->send_state();
                tx_pending = false;
            }
            System::dsb();
            System::get().wfi();
        }
    }

    /**
     * Callback for timer periods elapsing.
     * Timers have to be started with "HAL_TIM_Base_Start_IT" for this interrupt to work for them.
     * @param htim The timer whose period elapsed
     */
    auto timer_elapsed_callback(TIM_HandleTypeDef const* htim) -> void {
        if (!initialized) return;
        if (htim == TX_TIM) {
            tx_pending = true;
        }
    }

    /**
     * Callback enabling asynchronous serial logs via UART/DMA.
     * @param huart UART handle from callback
     */
    auto uart_tx_callback(UART_HandleTypeDef const* huart) -> void {
        if (huart == LPUART_1) {
            lpuart->handle_tx_complete();
        }
    }

} // namespace mrover

extern "C" {

void PostInit() {
    mrover::init();
}

void Loop() {
    mrover::loop();
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
// void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c) {}
// void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {}
// void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {}
}
