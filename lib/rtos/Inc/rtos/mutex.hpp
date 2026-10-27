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

    /**
     * Scoped lock. Only locks once the scheduler runs: before that there is a single thread of
     * execution, and a FreeRTOS mutex needs a running task to own it. Never use from an ISR.
     */
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

    // a driver's intrinsic lock: drivers lock it unconditionally, it compiles away without an RTOS
    using PeripheralMutex = Mutex;
#else  // MROVER_USE_RTOS
    class __attribute__((unavailable("enable rtos to use mrover::Mutex"))) Mutex {
    public:
        template<typename... Args>
        explicit Mutex(Args&&... args) {}
    };

    // without an RTOS a driver has one thread of execution, so its lock is free
    struct PeripheralMutex {
        explicit PeripheralMutex(char const* = nullptr) {}
    };

    struct MutexGuard {
        explicit MutexGuard(PeripheralMutex&) {}
    };
#endif // MROVER_USE_RTOS

} // namespace mrover
