#pragma once

#include <tuple>


namespace mrover {

    enum struct mode_t : uint8_t {
        STOPPED = 0,
        FAULT = 1,
        THROTTLE = 5,
        POSITION = 6,
        VELOCITY = 7,
    };

    // TODO(eric): update with flash PR
    template<typename T>
    struct reg_t {
        using value_t = T;

        std::string_view name;
        uint16_t addr{};
        value_t value;
        static consteval size_t size() { return sizeof(T); }
        [[nodiscard]] constexpr uint16_t reg() const { return addr; }
    };

    // TODO(eric): update with flash PR
    struct bmc_config_t {
        static constexpr reg_t<uint8_t> CAN_ID{"can_id", 0x00, 0x00};
        static constexpr reg_t<uint8_t> LIMITS_ENABLED{"limits_enabled", 0x01};
        static constexpr reg_t<uint16_t> INT_VALUE{"int_value", 0x02};
        static constexpr reg_t<float> FLOAT_VALUE{"float_value", 0x04};

        static constexpr auto all() {
            return std::make_tuple(CAN_ID, LIMITS_ENABLED, INT_VALUE, FLOAT_VALUE);
        }

    };

    /**
     * Get the BMC CAN settings.
     *
     * Delay compensation needs to be enabled to allow BRS.
     *
     * @return CAN option for BMC
     */
    inline auto get_can_options() -> FDCAN::Options {
        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        return can_opts;
    }

} // namespace mrover
