#pragma once

#include "stm32g4xx_hal_tim.h"
#include "AutonLED.hpp"
#include <MRoverCAN.hpp>

namespace mrover {
    class PDLB {
    private:
        AutonLED m_auton_led;
        TIM_HandleTypeDef* m_blink_tim;
        Pin m_can_tx{};
        Pin m_can_rx{};
        MRoverCANHandler m_can_handler{};

    public:
        PDLB() = default;

        PDLB (AutonLED& auton_led_in, TIM_HandleTypeDef* blink_tim_in, Pin& can_tx_in, Pin& can_rx_in, MRoverCANHandler& can_handler_in)
            : m_auton_led{auton_led_in}, m_blink_tim{blink_tim_in}, m_can_tx{can_tx_in}, m_can_rx{can_rx_in}, m_can_handler{can_handler_in} {}

        void blink() {
            m_auton_led.blink();
        }

        void set_led (bool red, bool green, bool blue, bool blinking) {
            if (blinking) {
                __HAL_TIM_SET_COUNTER(m_blink_tim, 0);
                HAL_TIM_Base_Start_IT(m_blink_tim);
            } else {
                HAL_TIM_Base_Stop_IT(m_blink_tim);
            }

            m_auton_led.change_state(red, green, blue, blinking);
        }

        static void reset() {
            HAL_DeInit();
            NVIC_SystemReset();
        }

        template<typename T>
        void handle(T const& _) {
        }

        void handle(const PDLBResetCommand& cmd) {
            if (cmd.reset)
                reset();
        }

        void handle(const AutonLEDCommand& cmd) {
            set_led(cmd.red, cmd.green, cmd.blue, cmd.blinking);
        }

        void handle_request() {
            auto const recv = m_can_handler.receive();
            if (recv) {
                m_can_rx.set();
                std::visit([this](auto&& value) { handle(value); }, *recv);
                m_can_rx.reset();
            }
        }
    };
} // namespace mrover