#pragma once

#include <serial/fdcan.hpp>
#include <serial/uart.hpp>
#include <tuple>

#include <MRoverCAN.hpp>
#include <hw/flash.hpp>

namespace mrover {
    struct abs_config_t {
        static inline void* flash_ptr = nullptr;

        FDCAN::Filter can_node_filter{};

        reg_t<uint8_t> CAN_ID{0x00};
        reg_t<uint8_t> HOST_CAN_ID{0x01};
        reg_t<uint8_t> NOISE_MARGIN{0x02};
        reg_t<uint8_t> USER_REG{0x03};
        reg_t<float> OUTPUT_SCALAR{0x04};
        reg_t<float> POSITION_OFFSET{0x08};
        reg_t<float> POLL_FREQUENCY{0x0C};
        reg_t<float> PUBLISH_FREQUENCY{0x10};

        using can_id = field_t<&abs_config_t::CAN_ID, 0, 8>;
        using host_can_id = field_t<&abs_config_t::HOST_CAN_ID, 0, 8>;
        using noise_margin = field_t<&abs_config_t::NOISE_MARGIN, 0, 8>;
        using user_reg = field_t<&abs_config_t::USER_REG, 0, 8>;
        using output_scalar = field_t<&abs_config_t::OUTPUT_SCALAR>;
        using position_offset = field_t<&abs_config_t::POSITION_OFFSET>;
        using poll_frequency = field_t<&abs_config_t::POLL_FREQUENCY>;
        using publish_frequency = field_t<&abs_config_t::PUBLISH_FREQUENCY>;

        template<typename F>
        auto get() const { return F::get(*this); }

        template<typename F>
        void set(auto value) { F::set(*this, value); }

        constexpr auto all() {
            return std::forward_as_tuple(CAN_ID, HOST_CAN_ID, NOISE_MARGIN, USER_REG, OUTPUT_SCALAR, POSITION_OFFSET, POLL_FREQUENCY, PUBLISH_FREQUENCY);
        }

        constexpr auto all() const {
            return std::forward_as_tuple(CAN_ID, HOST_CAN_ID, NOISE_MARGIN, USER_REG, OUTPUT_SCALAR, POSITION_OFFSET, POLL_FREQUENCY, PUBLISH_FREQUENCY);
        }

        auto set_raw(uint8_t address, uint32_t const raw) -> bool {
            bool found = false;

            std::apply(
                    [&](auto const&... reg) -> void {
                        (
                                [&] -> void {
                                    if (reg.addr == address) {
                                        using T = std::remove_reference_t<decltype(reg)>::value_t;
                                        reg.write(*this, from_raw<T>(raw));
                                        found = true;
                                    }
                                }(),
                                ...);
                    },
                    all());

            return found;
        }

        auto get_raw(uint8_t address, uint32_t& raw) const -> bool {
            bool found = false;
            std::apply([&](auto const&... reg) -> void {
                ((reg.addr == address ? (raw = to_raw(reg.value.value_or(0)), found = true) : false), ...);
            },
                       all());
            return found;
        }

        // stm32 g431cbt6
        struct mem_layout {
            static constexpr uint32_t FLASH_BEGIN_ADDR = 0x08000000;
            static constexpr uint32_t FLASH_END_ADDR = 0x0801FFFF;
            static constexpr int PAGE_SIZE = 2048;
            static constexpr int NUM_PAGES = 64;
        };

        static consteval auto size_bytes() -> uint16_t {
            return validated_config_t<abs_config_t>::size_bytes();
        }
    };

    /**
     * Get the ABS CAN settings.
     *
     * Delay compensation needs to be enabled to allow BRS.
     *
     * @return CAN options for ABS
     */
    inline auto get_can_options(abs_config_t* config) -> FDCAN::Options {
        config->can_node_filter.id1 = config->get<abs_config_t::can_id>();
        config->can_node_filter.id2 = CAN_DEST_ID_MASK;
        config->can_node_filter.id_type = FDCAN::FilterIdType::Extended;
        config->can_node_filter.action = FDCAN::FilterAction::Accept;
        config->can_node_filter.mode = FDCAN::FilterMode::Mask;

        FDCAN::FilterConfig filter;
        filter.begin = &config->can_node_filter;
        filter.end = &config->can_node_filter + 1;
        filter.global_non_matching_std_action = FDCAN::FilterAction::Reject;
        filter.global_non_matching_ext_action = FDCAN::FilterAction::Reject;

        auto can_opts = FDCAN::Options{};
        can_opts.delay_compensation = true;
        can_opts.tdc_offset = 13;
        can_opts.tdc_filter = 1;
        can_opts.filter_config = filter;
        return can_opts;
    }

    /**
     * Get the ABS UART settings.
     *
     * DMA must be enabled to allow non-blocking logging functionality.
     *
     * @return UART options for ABS
     */
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = true;
        return options;
    }

    /**
     * Get the ABS SPI settings.
     *
     * DMA must be enabled to allow non-blocking logging functionality.
     *
     * @return SPI options for ABS
     */
    inline auto get_spi_options() -> SPI::options_t {
        SPI::options_t options;
        options.use_dma = false;
        return options;
    }

} // namespace mrover
