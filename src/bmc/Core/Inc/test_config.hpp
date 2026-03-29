struct bmc_config_t {

static inline void* flash_ptr = nullptr;

    FDCAN::Filter can_node_filters[3];

    reg_t<uint8_t> CAN_ID{0x0};
    reg_t<uint8_t> HOST_CAN_ID{0x1};
    reg_t<uint8_t> SYS_CFG{0x2};
    reg_t<uint8_t> LIMIT_CFG{0x3};
    reg_t<float> QUAD_CPR{0x4};
    reg_t<float> GEAR_RATIO{0x8};
    reg_t<float> ROTOR_OUTPUT_RATIO{0xc};
    reg_t<float> LIMIT_A_POSITION{0x10};
    reg_t<float> LIMIT_B_POSITION{0x14};
    reg_t<float> MAX_PWM{0x18};
    reg_t<float> MIN_POS{0x1c};
    reg_t<float> MAX_POS{0x20};
    reg_t<float> MIN_VEL{0x24};
    reg_t<float> MAX_VEL{0x28};
    reg_t<float> POS_K_P{0x2c};
    reg_t<float> POS_K_I{0x30};
    reg_t<float> POS_K_D{0x34};
    reg_t<float> POS_K_F{0x38};
    reg_t<float> VEL_K_P{0x3c};
    reg_t<float> VEL_K_I{0x40};
    reg_t<float> VEL_K_D{0x44};
    reg_t<float> VEL_K_F{0x48};

    using can_id = field_t<&bmc_config_t::CAN_ID, 0, 8>;
    using host_can_id = field_t<&bmc_config_t::HOST_CAN_ID, 0, 8>;

    using motor_en = field_t<&bmc_config_t::SYS_CFG, 0>;
    using motor_inv = field_t<&bmc_config_t::SYS_CFG, 1>;
    using quad_en = field_t<&bmc_config_t::SYS_CFG, 2>;
    using quad_phase = field_t<&bmc_config_t::SYS_CFG, 3>;

    using lim_a_en = field_t<&bmc_config_t::LIMIT_CFG, 0>;
    using lim_a_active_high = field_t<&bmc_config_t::LIMIT_CFG, 1>;
    using lim_a_is_forward = field_t<&bmc_config_t::LIMIT_CFG, 2>;
    using lim_a_use_readjust = field_t<&bmc_config_t::LIMIT_CFG, 3>;
    using lim_b_en = field_t<&bmc_config_t::LIMIT_CFG, 4>;
    using lim_b_active_high = field_t<&bmc_config_t::LIMIT_CFG, 5>;
    using lim_b_is_forward = field_t<&bmc_config_t::LIMIT_CFG, 6>;
    using lim_b_use_readjust = field_t<&bmc_config_t::LIMIT_CFG, 7>;

    using quad_cpr = field_t<&bmc_config_t::QUAD_CPR>;
    using gear_ratio = field_t<&bmc_config_t::GEAR_RATIO>;
    using rotor_output_ratio = field_t<&bmc_config_t::ROTOR_OUTPUT_RATIO>;
    using limit_a_position = field_t<&bmc_config_t::LIMIT_A_POSITION>;
    using limit_b_position = field_t<&bmc_config_t::LIMIT_B_POSITION>;
    using max_pwm = field_t<&bmc_config_t::MAX_PWM>;
    using min_pos = field_t<&bmc_config_t::MIN_POS>;
    using max_pos = field_t<&bmc_config_t::MAX_POS>;
    using min_vel = field_t<&bmc_config_t::MIN_VEL>;
    using max_vel = field_t<&bmc_config_t::MAX_VEL>;
    using pos_k_p = field_t<&bmc_config_t::POS_K_P>;
    using pos_k_i = field_t<&bmc_config_t::POS_K_I>;
    using pos_k_d = field_t<&bmc_config_t::POS_K_D>;
    using pos_k_f = field_t<&bmc_config_t::POS_K_F>;
    using vel_k_p = field_t<&bmc_config_t::VEL_K_P>;
    using vel_k_i = field_t<&bmc_config_t::VEL_K_I>;
    using vel_k_d = field_t<&bmc_config_t::VEL_K_D>;
    using vel_k_f = field_t<&bmc_config_t::VEL_K_F>;

    constexpr auto all() {
        return std::forward_as_tuple(
            CAN_ID, HOST_CAN_ID, SYS_CFG, LIMIT_CFG, QUAD_CPR, GEAR_RATIO,
            ROTOR_OUTPUT_RATIO, LIMIT_A_POSITION, LIMIT_B_POSITION, MAX_PWM,
            MIN_POS, MAX_POS, MIN_VEL, MAX_VEL, POS_K_P, POS_K_I, POS_K_D,
            POS_K_F, VEL_K_P, VEL_K_I, VEL_K_D, VEL_K_F
        );
    }

    constexpr auto all () const {
        return std::forward_as_tuple(
            CAN_ID, HOST_CAN_ID, SYS_CFG, LIMIT_CFG, QUAD_CPR, GEAR_RATIO,
            ROTOR_OUTPUT_RATIO, LIMIT_A_POSITION, LIMIT_B_POSITION, MAX_PWM,
            MIN_POS, MAX_POS, MIN_VEL, MAX_VEL, POS_K_P, POS_K_I, POS_K_D,
            POS_K_F, VEL_K_P, VEL_K_I, VEL_K_D, VEL_K_F
    );
    }

    struct mem_layout {
        static constexpr uint32_t FLASH_BEGIN_ADDR = 0x8000000;
        static constexpr uint32_t FLASH_END_ADDR = 0x801ffff;
        static constexpr int PAGE_SIZE = 2048;
        static constexpr int NUM_PAGES = 64;
    };

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

    static consteval auto size_bytes() -> uint16_t {
        return validated_config_t<bmc_config_t>::size_bytes();
    }

};

inline auto get_can_options(bmc_config_t* config) -> FDCAN::Options {
    config->can_node_filters[0].id1 =  config->get<bmc_config_t::can_id()>;
    config->can_node_filters[0].id2 = CAN_DEST_ID_MASK;
    config->can_node_filters[0].id_type = FDCAN::FilterIdType::Extended;
    config->can_node_filters[0].action = FDCAN::ActionType::Accept;
    config->can_node_filters[0].mode = FDCAN::ActionType::Mask;

    config->can_node_filters[1].id1 = 71;
    config->can_node_filters[1].id2 = CAN_SRC_ID_MASK;
    config->can_node_filters[1].id_type = FDCAN::FilterIdType::Extended;
    config->can_node_filters[1].action = FDCAN::ActionType::Accept;
    config->can_node_filters[1].mode = FDCAN::ActionType::Mask;

    config->can_node_filters[2].id1 = 103;
    config->can_node_filters[2].id2 = CAN_SRC_ID_MASK;
    config->can_node_filters[2].id_type = FDCAN::FilterIdType::Extended;
    config->can_node_filters[2].action = FDCAN::ActionType::Accept;
    config->can_node_filters[2].mode = FDCAN::ActionType::Mask;

    FDCAN::FilterConfig filter;
    filter.begin = config->can_node_filters;
    filter.end = config->can_node_filters + 3
    filter.global_non_matching_std_action = FDCAN::FilterAction::Reject;
    filter.global_non_matching_ext_action = FDCAN::FilterAction::Reject;

    auto can_opts = FDCAN::Options{};
    can_opts.delay_compensation = true;
    can_opts.tdc_offset = 13;
    can_opts.tdc_filter = 1;
    can_opts.filter_config = filter;
    return can_opts;

}

