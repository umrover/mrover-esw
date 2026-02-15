#pragma once

#include <cstdint>
#include <random>
#include <string_view>
#include <type_traits>


namespace mrover {
    // stores value and register metadata
    template<typename T>
    struct reg_t {
        using value_t = T;
        std::string_view name;
        uint8_t addr{};
        value_t value;
        static consteval size_t size() { return sizeof(T); }
        [[nodiscard]] constexpr uint8_t reg() const { return addr; }
    };

    // field of a reg_t
    template<auto cfg_ptr_t, size_t bit = 0, size_t width = 1>
    struct field_t {
        template<typename C>
        using underlying_t = std::remove_reference_t<decltype(std::declval<C>().*cfg_ptr_t)>::value_t;

        static auto get(auto const& config) {
            using T = underlying_t<std::decay_t<decltype(config)>>;
            auto const& reg_item = (config.*cfg_ptr_t);

            if constexpr (std::is_floating_point_v<T>) {
                return reg_item.value;
            } else {
                if constexpr (width == 1) {
                    return static_cast<bool>((reg_item.value >> bit) & 1);
                } else {
                    constexpr T mask = (static_cast<T>(1) << width) - 1;
                    return static_cast<T>((reg_item.value >> bit) & mask);
                }
            }
        }

        static void set(auto& config, auto value) {
            using T = underlying_t<std::decay_t<decltype(config)>>;
            auto& reg_val = (config.*cfg_ptr_t).value;

            if constexpr (std::is_floating_point_v<T>) {
                reg_val = static_cast<T>(value);
            } else {
                constexpr T mask = ((static_cast<T>(1) << width) - 1) << bit;
                reg_val = (reg_val & ~mask) | ((static_cast<T>(value) << bit) & mask);
            }
        }
    };

    // science board config
    struct sb_config_t {
        reg_t<uint8_t> CAN_ID{"can_id", 0x00, 0x00};
        using can_id = field_t<&sb_config_t::CAN_ID, 0, 8>; // retrieves CAN_ID from bits 0-7 of CAN_ID register

        template<typename F>
        auto get() const { return F::get(*this); }
    };

    // retrieves science board uart options
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = false;
        return options;
    }

    // retrieves science board can options
    inline auto get_can_options() -> FDCAN::Options {
        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        return can_opts;
    }
} // namespace mrover
