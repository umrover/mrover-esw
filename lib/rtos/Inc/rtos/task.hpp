#pragma once

#include <cstddef>
#include <cstdint>

#ifdef MROVER_USE_RTOS
#include <array>

#include <FreeRTOS.h>
#include <task.h>

#include <cmsis_os2.h>
#endif // MROVER_USE_RTOS

namespace mrover {

#ifdef MROVER_USE_RTOS
    template<std::size_t StackBytes = 512>
    class Task {
        static_assert(StackBytes % sizeof(StackType_t) == 0, "StackBytes must be a multiple of the stack word size");
        static_assert(StackBytes >= configMINIMAL_STACK_SIZE * sizeof(StackType_t), "StackBytes is below configMINIMAL_STACK_SIZE");

        using entry_t = void (*)(void*);
        using simple_entry_t = void (*)();

        char const* m_name{};
        entry_t m_entry{};
        void* m_arg{};
        simple_entry_t m_simple{};
        osPriority_t m_priority{};
        osThreadId_t m_id{};

        StaticTask_t m_cb{};
        std::array<StackType_t, StackBytes / sizeof(StackType_t)> m_stack{};

    public:
        Task(char const* name, entry_t const entry, void* arg = nullptr, osPriority_t const priority = osPriorityNormal)
            : m_name{name}, m_entry{entry}, m_arg{arg}, m_priority{priority} {}

        Task(char const* name, simple_entry_t const entry, osPriority_t const priority = osPriorityNormal)
            : m_name{name}, m_entry{&Task::trampoline}, m_arg{this}, m_simple{entry}, m_priority{priority} {}

        Task(Task const&) = delete;
        auto operator=(Task const&) -> Task& = delete;
        Task(Task&&) = delete;
        auto operator=(Task&&) -> Task& = delete;

        auto start() -> bool {
            if (m_id) return true;

            osThreadAttr_t const attr{
                    .name = m_name,
                    .attr_bits = 0,
                    .cb_mem = &m_cb,
                    .cb_size = sizeof(m_cb),
                    .stack_mem = m_stack.data(),
                    .stack_size = m_stack.size() * sizeof(StackType_t),
                    .priority = m_priority,
            };
            m_id = osThreadNew(m_entry, m_arg, &attr);
            return m_id != nullptr;
        }

        auto suspend() const -> void {
            if (m_id) osThreadSuspend(m_id);
        }

        auto resume() const -> void {
            if (m_id) osThreadResume(m_id);
        }

        [[nodiscard]] auto stack_headroom() const -> std::size_t {
            if (!m_id) return 0;
            return uxTaskGetStackHighWaterMark(static_cast<TaskHandle_t>(m_id)) * sizeof(StackType_t);
        }

        [[nodiscard]] auto handle() const -> osThreadId_t {
            return m_id;
        }

    private:
        static auto trampoline(void* const self) -> void {
            static_cast<Task*>(self)->m_simple();
        }
    };
#else  // MROVER_USE_RTOS
    template<std::size_t StackBytes = 512>
    class __attribute__((unavailable("enable rtos to use mrover::Task"))) Task {
    public:
        template<typename... Args>
        explicit Task(Args&&... args) {}
    };
#endif // MROVER_USE_RTOS

} // namespace mrover
