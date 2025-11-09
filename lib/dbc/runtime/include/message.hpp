#pragma once

#include <cstdint>
#include <ostream>
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
        double m_minimum{};
        double m_maximum{};
        std::string m_unit{};
        std::string m_receiver{};
        MultiplexState m_multiplex_state = MultiplexState::None;
        std::string m_comment{};

    public:
        friend class CanMessageDescription;

        CanSignalDescription() = default;

        [[nodiscard]] auto name() const -> std::string;
        void set_name(std::string&& name);
        void set_name(std::string_view name);

        [[nodiscard]] auto bit_start() const -> uint16_t;
        void set_bit_start(uint16_t bit);

        [[nodiscard]] auto bit_length() const -> uint16_t;
        void set_bit_length(uint16_t length);

        [[nodiscard]] auto endianness() const -> Endianness;
        void set_endianness(Endianness endianness);

        [[nodiscard]] auto data_format() const -> DataFormat;
        void set_data_format(DataFormat format);

        [[nodiscard]] auto factor() const -> double;
        void set_factor(double factor);

        [[nodiscard]] auto offset() const -> double;
        void set_offset(double offset);

        [[nodiscard]] auto minimum() const -> double;
        void set_minimum(double minimum);

        [[nodiscard]] auto maximum() const -> double;
        void set_maximum(double maximum);

        [[nodiscard]] auto unit() const -> std::string;
        void set_unit(std::string&& unit);
        void set_unit(std::string_view unit);

        [[nodiscard]] auto receiver() const -> std::string;
        void set_receiver(std::string&& receiver);
        void set_receiver(std::string_view receiver);

        [[nodiscard]] auto multiplex_state() const -> MultiplexState;
        void set_multiplex_state(MultiplexState state);

        [[nodiscard]] auto comment() const -> std::string;
        void set_comment(std::string&& comment);
        void set_comment(std::string_view comment);

        [[nodiscard]] auto is_valid() const -> bool;

        friend auto operator<<(std::ostream& os, CanSignalDescription const& signal) -> std::ostream& {
            os << "Signal Name: \"" << signal.m_name << "\"\n";
            os << "  Bit Start: " << signal.m_bit_start << "\n";
            os << "  Bit Length: " << signal.m_bit_length << "\n";
            os << "  Endianness: " << (signal.m_endianness == Endianness::BigEndian ? "Big Endian" : "Little Endian") << "\n";
            os << "  Data Format: ";
            switch (signal.m_data_format) {
                case DataFormat::SignedInteger:
                    os << "Signed Integer";
                    break;
                case DataFormat::UnsignedInteger:
                    os << "Unsigned Integer";
                    break;
                case DataFormat::Float:
                    os << "Float";
                    break;
                case DataFormat::Double:
                    os << "Double";
                    break;
                case DataFormat::AsciiString:
                    os << "ASCII String";
                    break;
            }
            os << "\n";
            os << "  Factor: " << signal.m_factor << "\n";
            os << "  Offset: " << signal.m_offset << "\n";
            os << "  Minimum: " << signal.m_minimum << "\n";
            os << "  Maximum: " << signal.m_maximum << "\n";
            os << "  Unit: \"" << signal.m_unit << "\"\n";
            os << "  Receiver: \"" << signal.m_receiver << "\"\n";
            os << "  Multiplex State: ";
            switch (signal.m_multiplex_state) {
                case MultiplexState::None:
                    os << "None";
                    break;
                case MultiplexState::MultiplexorSwitch:
                    os << "Multiplexor Switch";
                    break;
                case MultiplexState::MultiplexedSignal:
                    os << "Multiplexed Signal";
                    break;
                case MultiplexState::SwitchAndSignal:
                    os << "Switch and Signal";
                    break;
            }
            os << "\n";
            os << "  Comment: \"" << signal.m_comment << "\"";
            return os;
        }

    private:
    };

    class CanMessageDescription {
        std::string m_name;
        uint32_t m_id;
        uint8_t m_length; // in bytes
        std::string m_transmitter;
        std::vector<CanSignalDescription> m_signals;
        std::string m_comment;

    public:
        [[nodiscard]] auto name() const -> std::string;
        void set_name(std::string&& name);
        void set_name(std::string_view name);

        [[nodiscard]] auto id() const -> uint32_t;
        void set_id(uint32_t id);

        [[nodiscard]] auto length() const -> uint8_t;
        void set_length(uint8_t length);

        [[nodiscard]] auto transmitter() const -> std::string;
        void set_transmitter(std::string&& transmitter);
        void set_transmitter(std::string_view transmitter);

        [[nodiscard]] auto signal_descriptions() const -> std::vector<CanSignalDescription>;
        [[nodiscard]] auto signal_description(std::string_view name) -> CanSignalDescription*;
        [[nodiscard]] auto signal_description(std::string_view name) const -> CanSignalDescription const*;

        void clear_signal_descriptions();
        void add_signal_description(CanSignalDescription signal);

        [[nodiscard]] auto comment() const -> std::string;
        void set_comment(std::string&& comment);
        void set_comment(std::string_view comment);

        [[nodiscard]] auto is_valid() const -> bool;

        friend auto operator<<(std::ostream& os, CanMessageDescription const& message) -> std::ostream& {
            os << "Message Name: " << message.m_name << "\n";
            os << "Message ID: " << message.m_id << "\n";
            os << "Message Length: " << static_cast<uint32_t>(message.m_length) << " bytes\n";
            os << "Transmitter: " << message.m_transmitter << "\n";
            os << "Comment: \"" << message.m_comment << "\"\n";
            os << "Signals:\n";
            for (auto const& signal: message.m_signals) {
                os << signal;
            }
            return os;
        }

    private:
    };


} // namespace mrover::dbc
