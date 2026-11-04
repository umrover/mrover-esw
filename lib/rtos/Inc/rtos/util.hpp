#pragma once

#ifdef MROVER_USE_RTOS
#include <cstdint>

#include <FreeRTOS.h>
#include <task.h>

#include CMSIS_device_header

namespace mrover {

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
