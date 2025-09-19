#pragma once

#include <optional>

#include <pin.hpp>

#include "main.h"


namespace mrover {
    class LimitSwitch {
    public:
        LimitSwitch() = default;

        explicit LimitSwitch(Pin const& pin) : m_pin{pin} {}

        auto initialize(bool enabled, bool active_high, bool used_for_readjustment,
                        bool limits_forward, float associated_position) -> void {
            m_valid = true;
            m_enabled = enabled;
            m_is_pressed = false;
            m_active_high = active_high;
            m_used_for_readjustment = used_for_readjustment;
            m_limits_forward = limits_forward;
            m_associated_position = associated_position;
        }

        auto update_limit_switch() -> void {
            // This suggests active low
            m_is_pressed = m_enabled ? m_active_high == m_pin.read() : false;
        }

        [[nodiscard]] auto pressed() const -> bool { return m_is_pressed; }

        [[nodiscard]] auto limit_forward() const -> bool {
            return m_valid && m_enabled && m_is_pressed && m_limits_forward;
        }

        [[nodiscard]] auto limit_backward() const -> bool {
            return m_valid && m_enabled && m_is_pressed && !m_limits_forward;
        }

        [[nodiscard]] auto get_readjustment_position() const
                -> std::optional<float> {
            // Returns std::null_opt if the value should not be readjusted
            return is_used_for_readjustment() && m_is_pressed
                           ? std::make_optional(m_associated_position)
                           : std::nullopt;
        }

        [[nodiscard]] auto is_used_for_readjustment() const -> bool {
            return m_valid && m_enabled && m_used_for_readjustment;
        }

        auto enable() -> void { m_enabled = true; }

        auto disable() -> void {
            m_enabled = false;
            m_is_pressed = false;
        }

    private:
        Pin m_pin;
        bool m_valid{};
        bool m_enabled{};
        bool m_is_pressed{};
        bool m_active_high{};
        bool m_used_for_readjustment{};
        bool m_limits_forward{};
        float m_associated_position{};
    };
} // namespace mrover
