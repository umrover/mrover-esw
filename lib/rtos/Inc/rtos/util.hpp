#pragma once

#ifdef MROVER_USE_RTOS
#include <cstdint>

#include <FreeRTOS.h>
#include <task.h>

#include CMSIS_device_header

namespace mrover {

    /**
     * Wrap every kernel object creation in one of these.
     *
     * Creating a kernel object enters a FreeRTOS critical section, and FreeRTOS deliberately leaves
     * BASEPRI raised after any critical section entered before the scheduler starts (uxCriticalNesting
     * starts at 0xaaaaaaaa). Without this, a single static Mutex (Logger has one) masks every interrupt
     * at or below configMAX_SYSCALL_INTERRUPT_PRIORITY -- HAL tick, UART, DMA, CAN rx -- until
     * osKernelStart(). Unmasking is safe then: no task has run, so no task can be blocked on an object
     * for an ISR to wake.
     */
    class CreationGuard {
        uint32_t const m_basepri = __get_BASEPRI();

    public:
        CreationGuard() = default;

        ~CreationGuard() {
            if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) __set_BASEPRI(m_basepri);
        }

        CreationGuard(CreationGuard const&) = delete;
        auto operator=(CreationGuard const&) -> CreationGuard& = delete;
    };

} // namespace mrover
#endif // MROVER_USE_RTOS
