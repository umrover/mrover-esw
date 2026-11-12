#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace mrover {

    template<typename T, std::size_t Size>
    class RunningMeanFilter {

        std::size_t m_index{};
        T m_running_sum{};
        std::array<T, Size> m_circular_buffer{};

    public:
        auto add_reading(T reading) -> void {
            m_running_sum += reading - std::exchange(m_circular_buffer[m_index], reading);
            m_index = (m_index + 1) % Size;
        }

        auto clear() -> void {
            m_circular_buffer.fill(T{});
            m_running_sum = T{};
            m_index = 0;
        }

        [[nodiscard]] auto get_filtered() const -> T {
            return m_running_sum / static_cast<T>(Size);
        }
    };

    template<typename T, double Alpha>
    class IIRFilter {

        T m_value{};

    public:
        auto add_reading(T reading) -> void {
            m_value = Alpha * reading + (1 - Alpha) * m_value;
        }

        [[nodiscard]] auto get_filtered() const -> T {
            return m_value;
        }

        auto clear() -> void {
            m_value = T{};
        }
    };

    template<typename T>
    class ButterworthFilter {
    };

} // namespace mrover
