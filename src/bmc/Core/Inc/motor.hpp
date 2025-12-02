#pragma once

#include <variant>
#include <hw/hbridge.hpp>
#include <CANBus1.hpp>

namespace mrover {

    class Motor {
        HBridge hbridge;

        template <typename T>
        auto handle(T const& value) -> void {
            Logger::get_instance()->info("Received Unhandled Message Type");
        }

        auto handle(BMCProbe const& msg) -> void {
            Logger::get_instance()->info("BMC Probed");
        }

    public:
        Motor() = default;

        explicit Motor(
            HBridge const& motor_driver
        ) :
            hbridge{motor_driver}
        {
            hbridge.change_max_pwm(100);
            hbridge.write(0);
        }

        auto receive(CANBus1Msg_t const& v) -> void {
            std::visit([this](auto&& value) {
                handle(value);
            }, v);
        }

        auto write(Percent const output) const -> void {
            hbridge.write(output);
        }
    };
} // namespace mrover
