#pragma once

#ifdef MROVER_USE_RTOS
#include <FreeRTOS.h>
#include <semphr.h>

#include <cmsis_os2.h>

#include <rtos/util.hpp>
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
            CreationGuard const guard{};
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

    class MutexGuard {
        Mutex& m_mutex;
        bool const m_locked;

    public:
        explicit MutexGuard(Mutex& mutex)
            : m_mutex{mutex}, m_locked{osKernelGetState() == osKernelRunning && mutex.lock()} {}

        ~MutexGuard() {
            if (m_locked) m_mutex.unlock();
        }

        MutexGuard(MutexGuard const&) = delete;
        auto operator=(MutexGuard const&) -> MutexGuard& = delete;
    };

    using PeripheralMutex = Mutex;
#else  // MROVER_USE_RTOS
    class __attribute__((unavailable("enable rtos to use mrover::Mutex"))) Mutex {
    public:
        template<typename... Args>
        explicit Mutex(Args&&... args) {}
    };

    struct PeripheralMutex {
        explicit PeripheralMutex(char const* = nullptr) {}
    };

    struct MutexGuard {
        explicit MutexGuard(PeripheralMutex&) {}
    };
#endif // MROVER_USE_RTOS

} // namespace mrover
