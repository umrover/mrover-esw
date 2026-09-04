#pragma once

#include <cstdint>

#ifdef MROVER_USE_RTOS
#include <FreeRTOS.h>
#include <semphr.h>

#include <cmsis_os2.h>
#endif // MROVER_USE_RTOS

namespace mrover {

#ifdef MROVER_USE_RTOS
    class Semaphore {
        StaticSemaphore_t m_cb{};
        osSemaphoreId_t m_id{};

    public:
        explicit Semaphore(uint32_t const max_count = 1, uint32_t const initial_count = 0, char const* name = nullptr) {
            osSemaphoreAttr_t const attr{
                    .name = name,
                    .attr_bits = 0,
                    .cb_mem = &m_cb,
                    .cb_size = sizeof(m_cb),
            };
            m_id = osSemaphoreNew(max_count, initial_count, &attr);
        }

        Semaphore(Semaphore const&) = delete;
        auto operator=(Semaphore const&) -> Semaphore& = delete;
        Semaphore(Semaphore&&) = delete;
        auto operator=(Semaphore&&) -> Semaphore& = delete;

        ~Semaphore() {
            if (m_id) osSemaphoreDelete(m_id);
        }

        [[nodiscard]] auto acquire(uint32_t const timeout_ms = osWaitForever) -> bool {
            return osSemaphoreAcquire(m_id, timeout_ms) == osOK;
        }

        [[nodiscard]] auto try_acquire() -> bool {
            return osSemaphoreAcquire(m_id, 0U) == osOK;
        }

        auto release() -> bool {
            return osSemaphoreRelease(m_id) == osOK;
        }

        [[nodiscard]] auto count() const -> uint32_t {
            return osSemaphoreGetCount(m_id);
        }

        [[nodiscard]] auto handle() const -> osSemaphoreId_t {
            return m_id;
        }
    };
#else  // MROVER_USE_RTOS
    class __attribute__((unavailable("enable rtos to use mrover::Semaphore"))) Semaphore {
    public:
        template<typename... Args>
        explicit Semaphore(Args&&... args) {}
    };
#endif // MROVER_USE_RTOS

} // namespace mrover
