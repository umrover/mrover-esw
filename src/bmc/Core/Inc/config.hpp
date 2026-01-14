#pragma once

#include <bit>
#include <cstdint>
#include <tuple>
#include <type_traits>

#include <adc.hpp>
#include <hw/ad8418a.hpp>


namespace mrover {

    enum struct mode_t : uint8_t {
        STOPPED = 0,
        FAULT = 1,
        THROTTLE = 5,
        POSITION = 6,
        VELOCITY = 7,
    };

    template<typename T>
    static auto from_raw(uint32_t raw) -> T {
        static_assert(std::is_trivially_copyable_v<T>);
        if constexpr (sizeof(T) == sizeof(uint32_t)) {
            return std::bit_cast<T>(raw);
        } else {
            return static_cast<T>(raw);
        }
    }

    template<typename T>
    static auto to_raw(T value) -> uint32_t {
        static_assert(std::is_trivially_copyable_v<T>);
        if constexpr (sizeof(T) == sizeof(uint32_t)) {
            return std::bit_cast<uint32_t>(value);
        } else {
            return static_cast<uint32_t>(value);
        }
    }

    template<typename T>
    struct reg_t {
        using value_t = T;
        std::string_view name;
        uint8_t addr{};
        value_t value;
        static consteval size_t size() { return sizeof(T); }
        [[nodiscard]] constexpr uint8_t reg() const { return addr; }
    };

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

    struct bmc_config_t {
        reg_t<uint8_t> CAN_ID{"can_id", 0x00, 0x00};
        reg_t<uint8_t> SYS_CFG{"system_configuration", 0x01, 0x00};
        reg_t<uint8_t> LIMIT_CFG{"limit_configuration", 0x02, 0x00};
        reg_t<uint8_t> USER_REG{"user_reg", 0x03, 0x00};
        reg_t<float> QUAD_RATIO{"quad_ratio", 0x04, 0.0f};
        reg_t<float> ABS_I2C_RATIO{"abs_i2c_ratio", 0x08, 0.0f};
        reg_t<float> ABC_I2C_OFFSET{"abs_i2c_offset", 0x0C, 0.0f};
        reg_t<float> ABS_SPI_RATIO{"abs_spi_ratio", 0x10, 0.0f};
        reg_t<float> ABS_SPI_OFFSET{"abs_spi_offset", 0x14, 0.0f};
        reg_t<float> GEAR_RATIO{"gear_ratio", 0x18, 0.0f};
        reg_t<float> LIMIT_FORWARD_POSITION{"limit_forward_readjust_pos", 0x1C, 0.0f};
        reg_t<float> LIMIT_BACKWARD_POSITION{"limit_backward_readjust_pos", 0x20, 0.0f};
        reg_t<float> MAX_PWM{"max_pwm", 0x24, 0.0f};
        reg_t<float> MIN_POS{"min_pos", 0x28, 0.0f};
        reg_t<float> MAX_POS{"max_pos", 0x2C, 0.0f};
        reg_t<float> MIN_VEL{"min_vel", 0x30, 0.0f};
        reg_t<float> MAX_VEL{"max_vel", 0x34, 0.0f};
        reg_t<float> K_P{"kp", 0x38, 0.0f};
        reg_t<float> K_I{"ki", 0x3C, 0.0f};
        reg_t<float> K_D{"kd", 0x40, 0.0f};
        reg_t<float> K_F{"kf", 0x44, 0.0f};

        using can_id = field_t<&bmc_config_t::CAN_ID, 0, 8>;

        using motor_en = field_t<&bmc_config_t::SYS_CFG, 0>;
        using motor_inv = field_t<&bmc_config_t::SYS_CFG, 1>;
        using quad_en = field_t<&bmc_config_t::SYS_CFG, 2>;
        using quad_phase = field_t<&bmc_config_t::SYS_CFG, 3>;
        using abs_i2c_en = field_t<&bmc_config_t::SYS_CFG, 4>;
        using abs_i2c_phase = field_t<&bmc_config_t::SYS_CFG, 5>;
        using abs_spi_en = field_t<&bmc_config_t::SYS_CFG, 6>;
        using abs_spi_phase = field_t<&bmc_config_t::SYS_CFG, 7>;

        using lim_a_en = field_t<&bmc_config_t::LIMIT_CFG, 0>;
        using lim_a_active_high = field_t<&bmc_config_t::LIMIT_CFG, 1>;
        using lim_a_is_forward = field_t<&bmc_config_t::LIMIT_CFG, 2>;
        using lim_a_use_readjust = field_t<&bmc_config_t::LIMIT_CFG, 3>;
        using lim_b_en = field_t<&bmc_config_t::LIMIT_CFG, 4>;
        using lim_b_active_high = field_t<&bmc_config_t::LIMIT_CFG, 5>;
        using lim_b_is_forward = field_t<&bmc_config_t::LIMIT_CFG, 6>;
        using lim_b_use_readjust = field_t<&bmc_config_t::LIMIT_CFG, 7>;

        using quad_ratio = field_t<&bmc_config_t::QUAD_RATIO>;
        using abs_i2c_ratio = field_t<&bmc_config_t::ABS_I2C_RATIO>;
        using abs_i2c_offset = field_t<&bmc_config_t::ABC_I2C_OFFSET>;
        using abs_spi_ratio = field_t<&bmc_config_t::ABS_SPI_RATIO>;
        using abs_spi_offset = field_t<&bmc_config_t::ABS_SPI_OFFSET>;
        using gear_ratio = field_t<&bmc_config_t::GEAR_RATIO>;
        using limit_forward_position = field_t<&bmc_config_t::LIMIT_FORWARD_POSITION>;
        using limit_backward_position = field_t<&bmc_config_t::LIMIT_BACKWARD_POSITION>;
        using max_pwm = field_t<&bmc_config_t::MAX_PWM>;
        using min_pos = field_t<&bmc_config_t::MIN_POS>;
        using max_pos = field_t<&bmc_config_t::MAX_POS>;
        using min_vel = field_t<&bmc_config_t::MIN_VEL>;
        using max_vel = field_t<&bmc_config_t::MAX_VEL>;
        using k_p = field_t<&bmc_config_t::K_P>;
        using k_i = field_t<&bmc_config_t::K_I>;
        using k_d = field_t<&bmc_config_t::K_D>;
        using kfd = field_t<&bmc_config_t::K_F>;

        template<typename F>
        auto get() const { return F::get(*this); }

        template<typename F>
        void set(auto value) { F::set(*this, value); }

        auto all() {
            return std::forward_as_tuple(
                    CAN_ID, SYS_CFG, LIMIT_CFG, USER_REG, QUAD_RATIO, ABS_I2C_RATIO,
                    ABC_I2C_OFFSET, ABS_SPI_RATIO, ABS_SPI_OFFSET, GEAR_RATIO,
                    LIMIT_FORWARD_POSITION, LIMIT_BACKWARD_POSITION, MAX_PWM,
                    MIN_POS, MAX_POS, MIN_VEL, MAX_VEL, K_P, K_I, K_D, K_F);
        }

        auto all() const {
            return std::forward_as_tuple(
                    CAN_ID, SYS_CFG, LIMIT_CFG, USER_REG, QUAD_RATIO, ABS_I2C_RATIO,
                    ABC_I2C_OFFSET, ABS_SPI_RATIO, ABS_SPI_OFFSET, GEAR_RATIO,
                    LIMIT_FORWARD_POSITION, LIMIT_BACKWARD_POSITION, MAX_PWM,
                    MIN_POS, MAX_POS, MIN_VEL, MAX_VEL, K_P, K_I, K_D, K_F);
        }

        auto set_raw(uint8_t address, uint32_t const raw) -> bool {
            bool updated = false;
            std::apply([&](auto&... reg) {
                ((reg.addr == address ? (reg.value = from_raw<typename std::decay_t<decltype(reg)>::value_t>(raw), updated = true) : false), ...);
            },
                       all());
            return updated;
        }

        auto get_raw(uint8_t address, uint32_t& raw) const -> bool {
            bool found = false;
            std::apply([&](auto const&... reg) {
                ((reg.addr == address ? (raw = to_raw(reg.value), found = true) : false), ...);
            },
                       all());
            return found;
        }
    };

    /**
     * Get the BMC CAN settings.
     *
     * Delay compensation needs to be enabled to allow BRS.
     *
     * @return CAN options for BMC
     */
    inline auto get_can_options() -> FDCAN::Options {
        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        return can_opts;
    }

    /**
     * Get the BMC UART settings.
     *
     * DMA must be enabled to allow non-blocking logging functionality.
     *
     * @return UART options for BMC
     */
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = true;
        return options;
    }

    /**
     * Get the BMC ADC settings.
     *
     * DMA must be enabled to allow non-blocking ADC functionality.
     *
     * @return ADC options for BMC
     */
    inline auto get_adc_options() -> ADCBase::Options {
        ADCBase::Options options;
        options.use_dma = false; // TODO(eric) use dma for this
        return options;
    }

    inline auto get_current_sensor_options() -> AD8418A::Options {
        AD8418A::Options options;
        options.gain = 20.0f;
        options.shunt_resistance = 0.0005f;
        options.vref = 3.3f;
        options.vcm = options.vref / 2.0f;
        options.adc_resolution = 4095;
        return options;
    }

} // namespace mrover
