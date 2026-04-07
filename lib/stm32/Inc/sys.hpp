#pragma once

#ifdef STM32
#include "main.h"
#endif // STM32

namespace mrover {

    class System {
    public:
        struct options_t {
            options_t() {}
            bool sleep_on_wfi{true};
            bool enable_debug_sleep{true};
        };

        enum class fault_reason_t {
            UNKNOWN_ERROR = 0,
            HALT_ERROR,
            MALLOC_FAILED,
            HW_INIT_FAILED,
            ASSERT_FAILED,
            HARD_FAULT
        };

        System() = default;
        ~System() = default;

        explicit System(options_t const options) : m_options{options} {
            if (m_options.enable_debug_sleep) {
                // keep debugger attached when cpu goes to sleep
                HAL_DBGMCU_EnableDBGSleepMode();
            }
        }

        static auto reset() -> void {
            HAL_DeInit();
            NVIC_SystemReset();
        }


        // Wait For Interrupt
        auto wfi() const -> void {
            if (m_options.sleep_on_wfi) {
                HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            } else {
                __WFI();
            }
        }

        // Data Synchronization Barrier
        static auto dsb() -> void {
            __DSB();
        }

        // Instruction Synchronization Barrier
        static auto isb() -> void {
            __ISB();
        }

        static auto get_ticks() -> uint32_t {
            return HAL_GetTick();
        }

        static auto delay_ms(uint32_t const ms) -> void {
            HAL_Delay(ms);
        }

        static auto disable_interrupts() -> void {
            __disable_irq();
        }

        static auto enable_interrupts() -> void {
            __enable_irq();
        }

        class InterruptGuard {
        public:
            InterruptGuard() { disable_interrupts(); }
            ~InterruptGuard() { enable_interrupts(); }

            InterruptGuard(InterruptGuard const&) = delete;
            auto operator=(InterruptGuard const&) -> InterruptGuard& = delete;
            InterruptGuard(InterruptGuard&&) = delete;
            auto operator=(InterruptGuard&&) -> InterruptGuard& = delete;
        };

        static auto get_unique_id() -> uint32_t* {
            return reinterpret_cast<uint32_t*>(UID_BASE);
        }

        [[noreturn]] static auto fault(fault_reason_t reason = fault_reason_t::UNKNOWN_ERROR) -> void {
            disable_interrupts();

            // TODO(eric): store fail state to SRAM here

            for (;;) {
                __NOP();
            }
        }

        [[noreturn]] static auto handle_hard_fault(uint32_t const* fault_stack_address) -> void {
            uint32_t const volatile r0 = fault_stack_address[0];
            uint32_t const volatile r1 = fault_stack_address[1];
            uint32_t const volatile r2 = fault_stack_address[2];
            uint32_t const volatile r3 = fault_stack_address[3];
            uint32_t const volatile r12 = fault_stack_address[4];
            uint32_t const volatile lr = fault_stack_address[5];
            uint32_t const volatile pc = fault_stack_address[6]; // PC (on crash)
            uint32_t const volatile psr = fault_stack_address[7];

            (void) r0;
            (void) r1;
            (void) r2;
            (void) r3;
            (void) r12;
            (void) lr;
            (void) pc;
            (void) psr;

            fault(fault_reason_t::HARD_FAULT);
        }

    private:
        options_t m_options{};
    };

} // namespace mrover
