#pragma once

#include "serial/smbus.hpp"
#include "stm32g4xx_hal_def.h"
#include <MRoverCAN.hpp>
#include <adc.hpp>
#include <serial/fdcan.hpp>
#include <serial/uart.hpp>


namespace mrover {
    struct pdlb_config_t {
        // can filter
        FDCAN::Filter can_node_filter{};

        // CAN IDs
        uint8_t const PDLB_CAN_ID = 0x11;
        uint8_t const JETSON_CAN_ID = 0x10;
    };

    // retrieves science pdlb can options
    inline auto get_can_options(pdlb_config_t* config) -> FDCAN::Options {
        config->can_node_filter.id1 = config->PDLB_CAN_ID;
        config->can_node_filter.id2 = config->JETSON_CAN_ID;
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
} // namespace mrover
