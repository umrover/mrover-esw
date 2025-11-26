#pragma once

#include <hw/hbridge.hpp>

namespace mrover {

    class Motor {
        HBridge hbridge;

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

        auto write(Percent const output) const -> void {
            hbridge.write(output);
        }
    };
} // namespace mrover
