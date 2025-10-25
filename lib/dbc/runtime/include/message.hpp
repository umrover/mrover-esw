#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mrover::dbc {

    enum class Endianness {
        BigEndian = 0,
        LittleEndian = 1,
    };

    enum class DataFormat {
        SignedInteger = 0,
        UnsignedInteger = 1,
        Float = 2,
        Double = 3,
        AsciiString = 4,
    };

    enum class MultiplexState {
        None = 0x00,
        MultiplexorSwitch = 0x01,
        MultiplexedSignal = 0x02,
        SwitchAndSignal = MultiplexorSwitch | MultiplexedSignal, // if extended multiplexing is supported
    };

    class CanSignalDescription {
        std::string m_name{};
        uint16_t m_bit_start{};
        uint16_t m_bit_length{};
        Endianness m_endianness{};
        DataFormat m_data_format{};
        double m_factor{};
        double m_offset{};
        MultiplexState m_multiplex_state = MultiplexState::None;

    public:
        friend class CanMessageDescription;

        CanSignalDescription() = default;

        [[nodiscard]] auto get_name() const -> std::string;
        void set_name(std::string const& name);
        void set_name(std::string_view name);

        [[nodiscard]] auto get_bit_start() const -> uint16_t;
        void set_bit_start(uint16_t bit);

        [[nodiscard]] auto get_bit_length() const -> uint16_t;
        void set_bit_length(uint16_t length);

        [[nodiscard]] auto get_endianness() const -> Endianness;
        void set_endianness(Endianness endianness);

        [[nodiscard]] auto get_data_format() const -> DataFormat;
        void set_data_format(DataFormat format);

        [[nodiscard]] auto get_factor() const -> double;
        void set_factor(double factor);

        [[nodiscard]] auto get_offset() const -> double;
        void set_offset(double offset);

        [[nodiscard]] auto get_multiplex_state() const -> MultiplexState;
        void set_multiplex_state(MultiplexState state);

        [[nodiscard]] auto is_valid() const -> bool;

    private:
    };

    class CanMessageDescription {
        std::string m_name;
        uint32_t m_id;
        uint8_t m_length; // in bytes
        std::string m_transmitter;
        std::vector<CanSignalDescription> m_signals;

    public:
        [[nodiscard]] auto get_name() const -> std::string;
        void set_name(std::string const& name);
        void set_name(std::string_view name);

        [[nodiscard]] auto get_id() const -> uint32_t;
        void set_id(uint32_t id);

        [[nodiscard]] auto get_length() const -> uint8_t;
        void set_length(uint8_t length);

        [[nodiscard]] auto get_transmitter() const -> std::string;
        void set_transmitter(std::string const& transmitter);
        void set_transmitter(std::string_view transmitter);

        [[nodiscard]] auto get_signal_descriptions() const -> std::vector<CanSignalDescription>;
        [[nodiscard]] auto get_signal_description_by_name(std::string const& name) const -> CanSignalDescription;
        void clear_signal_descriptions();
        void add_signal_description(CanSignalDescription const& signal);

        [[nodiscard]] auto is_valid() const -> bool;

    private:
    };


} // namespace mrover::dbc
