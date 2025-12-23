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

        auto all() {
            return std::make_tuple(CAN_ID, SYS_CFG, LIMIT_CFG, USER_REG, QUAD_RATIO, ABS_I2C_RATIO,
                ABC_I2C_OFFSET, ABS_SPI_RATIO, ABS_SPI_OFFSET, GEAR_RATIO, LIMIT_FORWARD_POSITION,
                LIMIT_BACKWARD_POSITION, MAX_PWM, MIN_POS, MAX_POS, MIN_VEL, MAX_VEL, K_P, K_I, K_D, K_F);
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
