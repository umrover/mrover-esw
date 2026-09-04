#pragma once

#include <cstddef>
#include <cstdint>

#ifdef MROVER_USE_RTOS
#include <array>
#include <type_traits>

#include <FreeRTOS.h>
#include <queue.h>

#include <cmsis_os2.h>
#endif // MROVER_USE_RTOS

namespace mrover {

#ifdef MROVER_USE_RTOS
    template<typename T, std::size_t N>
    class Queue {
        static_assert(std::is_trivially_copyable_v<T>, "Queue<T> copies messages bytewise; T must be trivially copyable");
        static_assert(N > 0, "Queue depth must be non-zero");

        StaticQueue_t m_cb{};
        std::array<std::uint8_t, N * sizeof(T)> m_storage{};
        osMessageQueueId_t m_id{};

    public:
        explicit Queue(char const* name = nullptr) {
            osMessageQueueAttr_t const attr{
                    .name = name,
                    .attr_bits = 0,
                    .cb_mem = &m_cb,
                    .cb_size = sizeof(m_cb),
                    .mq_mem = m_storage.data(),
                    .mq_size = m_storage.size(),
            };
            m_id = osMessageQueueNew(N, sizeof(T), &attr);
        }

        Queue(Queue const&) = delete;
        auto operator=(Queue const&) -> Queue& = delete;
        Queue(Queue&&) = delete;
        auto operator=(Queue&&) -> Queue& = delete;

        ~Queue() {
            if (m_id) osMessageQueueDelete(m_id);
        }

        auto put(T const& msg, uint32_t const timeout_ms = osWaitForever) -> bool {
            return osMessageQueuePut(m_id, &msg, 0U, timeout_ms) == osOK;
        }

        auto put_from_isr(T const& msg) -> bool {
            return osMessageQueuePut(m_id, &msg, 0U, 0U) == osOK;
        }

        [[nodiscard]] auto get(T& out, uint32_t const timeout_ms = osWaitForever) -> bool {
            return osMessageQueueGet(m_id, &out, nullptr, timeout_ms) == osOK;
        }

        [[nodiscard]] auto get_from_isr(T& out) -> bool {
            return osMessageQueueGet(m_id, &out, nullptr, 0U) == osOK;
        }

        [[nodiscard]] auto count() const -> std::size_t {
            return osMessageQueueGetCount(m_id);
        }

        [[nodiscard]] auto empty() const -> bool {
            return count() == 0;
        }

        [[nodiscard]] auto full() const -> bool {
            return osMessageQueueGetSpace(m_id) == 0;
        }

        [[nodiscard]] auto handle() const -> osMessageQueueId_t {
            return m_id;
        }
    };
#else  // MROVER_USE_RTOS
    template<typename T, std::size_t N>
    class __attribute__((unavailable("enable rtos to use mrover::Queue"))) Queue {
    public:
        template<typename... Args>
        explicit Queue(Args&&... args) {}
    };
#endif // MROVER_USE_RTOS

} // namespace mrover
