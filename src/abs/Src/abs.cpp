#include "main.h"

#include <variant>
#include <cinttypes>
#include <serial/fdcan.hpp>
#include <serial/spi.hpp>
#include <serial/uart.hpp>
#include <logger.hpp>

#include "config.hpp"
#include <timer.hpp>
#include <hw/pin.hpp>
#include <hw/as5047u.hpp>


extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef hlpuart1;
extern SPI_HandleTypeDef hspi1;

extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;


namespace mrover {

    static constexpr FDCAN_HandleTypeDef* FDCAN_1 = &hfdcan1;
    static constexpr UART_HandleTypeDef* LPUART_1 = &hlpuart1;
    static constexpr SPI_HandleTypeDef* SPI_1 = &hspi1;

    static constexpr TIM_HandleTypeDef* ENCODER_TIM = &htim16; // default 30 Hz
    static constexpr TIM_HandleTypeDef* PUBLISH_TIM = &htim17; // default 30 Hz

    abs_config_t config;
    bool volatile initialized = false;
    bool volatile enc_request = false;
    bool volatile pending_pub = false;

    // Peripherals
    FDCAN fdcan;
    UART lpuart;
    SPI spi;

    // Timers
    std::optional<Timer> encoder_tim;
    std::optional<Timer> publish_tim;

    // Hardware Units
    std::optional<Pin> can_tx;
    std::optional<Pin> can_rx;
    std::optional<MRoverCANHandler> can_receiver;
    std::optional<AS5047U> encoder;

    auto init() -> void {
        __disable_irq();
        HAL_DBGMCU_EnableDBGSleepMode();

        // initialize peripherals
        fdcan = FDCAN{FDCAN_1, get_can_options(&config)};
        lpuart = UART{LPUART_1, get_uart_options()};
        spi = SPI{SPI_1, get_spi_options()};

        // initialize logger
        Logger::init(&lpuart);

        // setup timers
        encoder_tim.emplace(ENCODER_TIM, true);  // encoder poll timer (on interrupt)
        publish_tim.emplace(PUBLISH_TIM, true);  // can publish timer (on interrupt)

        // override default timer frequencies based on config
        encoder_tim->set_frequency(config.get<abs_config_t::poll_frequency>());
        publish_tim->set_frequency(config.get<abs_config_t::publish_frequency>());

        // setup debug LEDs
        can_tx.emplace(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin);
        can_rx.emplace(CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin);

        // initialize fdcan
        can_receiver = MRoverCANHandler{&fdcan};

        // initialize encoder
        encoder.emplace(
            &spi,
            config.get<abs_config_t::output_scalar>(),
            config.get<abs_config_t::position_offset>(),
            config.get<abs_config_t::noise_margin>()
        );

        Logger::instance().info("Initialized ABS Encoder %x", config.get<abs_config_t::can_id>());
        __enable_irq();
    }

    /**
     * Send a CAN message defined in MRoverCAN.dbc on the bus.
     * @param msg CAN message to send
     */
    auto send_can_message(MRoverCANMsg_t const& msg) -> void {
        if (!initialized) return;
        static std::optional<uint8_t> can_id = std::nullopt;
        static std::optional<uint8_t> host_can_id = std::nullopt;

        if (!can_id.has_value()) can_id = config.get<abs_config_t::can_id>();
        if (!host_can_id.has_value()) host_can_id = config.get<abs_config_t::host_can_id>();

        can_tx->set();
        can_receiver->send(msg, can_id.value(), host_can_id.value());
        can_tx->reset();
    }

    template<typename T>
    auto handle(T const& _) -> void {
    }

    auto handle(ESWProbe const& msg) -> void {
        // acknowledge probe
        send_can_message(ESWAck{msg.data});
    }

    auto handle(ESWConfigCmd const& msg) -> void {
        // input can either be a request to set a value (apply is set) or read a value (apply not set)
        if (msg.apply) {
            if (config.set_raw(msg.address, msg.value)) {
                // re-initialize after configuration is modified
                init();
            }
        } else {
            // send data back as an acknowledgement of the request
            if (uint32_t val{}; config.get_raw(msg.address, val)) {
                send_can_message(ESWAck{val});
            }
        }
    }

    auto handle(ABSResetCmd const& msg) -> void {
        // TODO(eric) core reset functionality when science makes it into main
    }

    auto handle(ABSZeroCmd const& msg) -> void {
        encoder->set_zero_offset(msg.offset);
    }

    /**
     * Receive and parse a CAN message over the bus.
     * Message should be of a type defined in CANBus1.dbc
     */
    auto receive_can_message() -> void {
        if (!initialized) return;

        while (fdcan.messages_to_process() > 0) {
            if (auto const recv = can_receiver->receive(); recv) {
                can_rx->set();
                auto const& msg = *recv;
                std::visit([](auto&& value) -> auto {
                    handle(value);
                },
                           msg);
                can_rx->reset();
            }
        }
    }

    [[noreturn]] auto loop() -> void {
        for (;;) {
            if (enc_request) {
                encoder->update();
                enc_request = false;
            }
            if (pending_pub) {
                send_can_message(ABSEncoderState{encoder->get_position(), encoder->get_velocity()});
                pending_pub = false;
            }
            __DSB();
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
        }
    }

    /**
    * Callback for timer periods elapsing.
    * Timers have to be started with "HAL_TIM_Base_Start_IT" for this interrupt to work for them.
    * @param htim The timer whose period elapsed
    */
    auto timer_elapsed_callback(TIM_HandleTypeDef const* htim) -> void {
        if (!initialized) return;
        if (htim == ENCODER_TIM) {
            enc_request = true;
        } else if (htim == PUBLISH_TIM) {
            pending_pub = true;
        }
    }

    auto spi_callback(SPI_HandleTypeDef const* hspi) -> void {
        if (SPI::s_dma_instance != nullptr && hspi == SPI::s_dma_instance->handle()) {
            SPI::s_dma_instance->handle_irq();
        }
    }

} // namespace mrover

extern "C" {

void Init() {
    mrover::init();
}

void Loop() {
    mrover::loop();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    mrover::timer_elapsed_callback(htim);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
    mrover::spi_callback(hspi);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs) {
    mrover::receive_can_message();
}

}
