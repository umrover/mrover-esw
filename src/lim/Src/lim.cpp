#include "main.h"
#include <MRoverCAN.hpp>
#include <hw/limit_switch.hpp>
#include <hw/pin.hpp>
#include <logger.hpp>
#include <optional>
#include <serial/fdcan.hpp>
#include <timer.hpp>

#include "main.h"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_tim.h"

//initialize peripherals
extern UART_HandleTypeDef hlpuart1;
extern FDCAN_HandleTypeDef hfdcan1;
extern TIM_HandleTypeDef htim17;


namespace mrover {

    static constexpr FDCAN_HandleTypeDef* FDCAN_1 = &hfdcan1;
    static constexpr UART_HandleTypeDef* LPUART_1 = &hlpuart1;
    static constexpr TIM_HandleTypeDef* PUBLISH_TIM = &htim17;


    bool volatile initialized = false;
    bool volatile pending_pub = false;
    bool lim_a_state;
    bool lim_b_state;

    //Peripherals
    FDCAN fdcan;
    UART lpuart;

    //Timers
    std::optional<Timer> publish_tim;

    //Hardware Units
    std::optional<LimitSwitch> lim_a;
    std::optional<LimitSwitch> lim_b;
    std::optional<Pin> can_tx;
    std::optional<Pin> can_rx;
    std::optional<MRoverCANHandler> can_receiver;


    auto send_can_message(MRoverCANMsg_t const& msg) -> void {
        if (!initialized) return;
        static std::optional<uint8_t> can_id = std::nullopt;
        static std::optional<uint8_t> host_can_id = std::nullopt;

        if (!can_id.has_value()) can_id = 0;
        if (!host_can_id.has_value()) host_can_id = 0;

        can_tx->set();
        can_receiver->send(msg, can_id.value(), host_can_id.value());
        can_tx->reset();
    }

    template<typename T>
    auto handle(T const& _) -> void {
    }

    auto handle(LIMResetState const& msg) -> void {
        lim_a->disable();
        lim_a->enable();
        lim_b->disable();
        lim_b->enable();
    }


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


    auto init() -> void {
        //initialize peripherals
        fdcan = FDCAN{FDCAN_1};
        lpuart = UART{LPUART_1};

        Logger::init(&lpuart);

        publish_tim.emplace(PUBLISH_TIM, true);
        //publish_tim->set_frequency();//config <-- set frequency is only in timer.hpp within fw/abs branch, do i need it in this branch?


        can_tx.emplace(CAN_TX_LED_GPIO_Port, CAN_TX_LED_Pin);
        can_rx.emplace(CAN_RX_LED_GPIO_Port, CAN_RX_LED_Pin);
        lim_a.emplace(Pin(LIMIT_A_GPIO_Port, LIMIT_A_Pin)); //init lim_a based on config values
        lim_b.emplace(Pin(LIMIT_B_GPIO_Port, LIMIT_B_Pin)); //init lim_b based on config values

        //initialize fdcan
        can_receiver = MRoverCANHandler{&fdcan};

        initialized = true;
    }

    auto loop() -> void {
        lim_a->update_limit_switch();
        lim_b->update_limit_switch();
        lim_a_state = lim_a->pressed();
        lim_b_state = lim_b->pressed();

        MRoverCANMsg_t msg = LIMSwitchState{
                lim_a_state,
                lim_b_state};

        if (pending_pub) {
            send_can_message(msg);
            pending_pub = false;
        }
    }

    auto timer_elapsed_callback(TIM_HandleTypeDef const* htim) -> void {
        if (!initialized) return;
        if (htim == PUBLISH_TIM) {
            pending_pub = true;
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
}
