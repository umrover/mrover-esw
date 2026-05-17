#pragma once

#include <serial/spi.hpp>
#include <serial/uart.hpp>

namespace mrover {
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = true;
        return options;
    }

    inline auto get_spi_options() -> SPI::options_t {
        SPI::options_t options;
        options.use_dma = true;
        return options;
    }

    inline auto get_sys_options() -> System::options_t {
        System::options_t options;
        options.sleep_on_wfi = true;
        return options;
    }
} // namespace mrover
