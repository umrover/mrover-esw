#pragma once

#include <bit>
#include <cstdint>
#include <tuple>
#include <type_traits>


namespace mrover {
    /**
     * Get the BMC UART settings.
     *
     * DMA must be enabled to allow non-blocking logging functionality.
     *
     * @return UART options for BMC
     */
    inline auto get_uart_options() -> UART::Options {
        UART::Options options;
        options.use_dma = false;
        return options;
    }
} // namespace mrover
