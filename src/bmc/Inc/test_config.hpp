struct test_config_t {
    reg_t<uint16_t> INT_REG{0x0};
    reg_t<float> FLOAT_REG{0x2};
    using field1 = field_t<&test_config_t::INT_REG, 0, 1>;
    using field2 = field_t<&test_config_t::INT_REG, 1, 1>;
    using field3 = field_t<&test_config_t::INT_REG, 2, 1>;
    using field4 = field_t<&test_config_t::INT_REG, 3, 1>;
    using field5 = field_t<&test_config_t::INT_REG, 4, 4>;
    using field6 = field_t<&test_config_t::INT_REG, 8, 8>;
    using float_reg = field_t<&test_config_t::FLOAT_REG>;
};
