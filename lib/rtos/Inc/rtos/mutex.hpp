#pragma once

#ifdef MROVER_USE_RTOS
#include <FreeRTOS.h>
#include <semphr.h>

#include <cmsis_os2.h>
#endif // MROVER_USE_RTOS

namespace mrover {

#ifdef MROVER_USE_RTOS
    class Mutex {
        StaticSemaphore_t m_cb{};
        osMutexId_t m_id{};

    public:
        explicit Mutex(char const* name = nullptr) {
            osMutexAttr_t const attr{
                    .name = name,
                    .attr_bits = osMutexPrioInherit,
                    .cb_mem = &m_cb,
                    .cb_size = sizeof(m_cb),
            };
            m_id = osMutexNew(&attr);
        }

        Mutex(Mutex const&) = delete;
        auto operator=(Mutex const&) -> Mutex& = delete;
        Mutex(Mutex&&) = delete;
        auto operator=(Mutex&&) -> Mutex& = delete;

        ~Mutex() {
            if (m_id) osMutexDelete(m_id);
        }

        auto lock(uint32_t const timeout_ms = osWaitForever) -> bool {
            return osMutexAcquire(m_id, timeout_ms) == osOK;
        }

        auto try_lock() -> bool {
            return osMutexAcquire(m_id, 0U) == osOK;
        }

        auto unlock() -> void {
            osMutexRelease(m_id);
        }

        [[nodiscard]] auto handle() const -> osMutexId_t {
            return m_id;
        }
    };
#else  // MROVER_USE_RTOS
    class __attribute__((unavailable("enable rtos to use mrover::Mutex"))) Mutex {
    public:
        template<typename... Args>
        explicit Mutex(Args&&... args) {}
    };
#endif // MROVER_USE_RTOS

} // namespace mrover
