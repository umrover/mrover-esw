#include "message.hpp"

#include <algorithm>
#include <utility>

namespace mrover::dbc {

    /**
     * CanSignalDescription
     */

    [[nodiscard]] auto CanSignalDescription::name() const -> std::string { return m_name; }
    void CanSignalDescription::set_name(std::string&& name) { m_name = std::move(name); }
    void CanSignalDescription::set_name(std::string_view name) { m_name = name; }

    [[nodiscard]] auto CanSignalDescription::bit_start() const -> uint16_t { return m_bit_start; }
    void CanSignalDescription::set_bit_start(uint16_t bit) { m_bit_start = bit; }

    [[nodiscard]] auto CanSignalDescription::bit_length() const -> uint16_t { return m_bit_length; }
    void CanSignalDescription::set_bit_length(uint16_t length) { m_bit_length = length; }

    [[nodiscard]] auto CanSignalDescription::endianness() const -> Endianness { return m_endianness; }
    void CanSignalDescription::set_endianness(Endianness endianness) { m_endianness = endianness; }

    [[nodiscard]] auto CanSignalDescription::data_format() const -> DataFormat { return m_data_format; }
    void CanSignalDescription::set_data_format(DataFormat format) { m_data_format = format; }

    [[nodiscard]] auto CanSignalDescription::factor() const -> double { return m_factor; }
    void CanSignalDescription::set_factor(double factor) { m_factor = factor; }

    [[nodiscard]] auto CanSignalDescription::offset() const -> double { return m_offset; }
    void CanSignalDescription::set_offset(double offset) { m_offset = offset; }

    [[nodiscard]] auto CanSignalDescription::minimum() const -> double { return m_minimum; }
    void CanSignalDescription::set_minimum(double minimum) { m_minimum = minimum; }

    [[nodiscard]] auto CanSignalDescription::maximum() const -> double { return m_maximum; }
    void CanSignalDescription::set_maximum(double maximum) { m_maximum = maximum; }

    [[nodiscard]] auto CanSignalDescription::unit() const -> std::string { return m_unit; }
    void CanSignalDescription::set_unit(std::string&& unit) { m_unit = std::move(unit); }
    void CanSignalDescription::set_unit(std::string_view unit) { m_unit = unit; }

    [[nodiscard]] auto CanSignalDescription::receiver() const -> std::string { return m_receiver; }
    void CanSignalDescription::set_receiver(std::string&& receiver) { m_receiver = std::move(receiver); }
    void CanSignalDescription::set_receiver(std::string_view receiver) { m_receiver = receiver; }

    [[nodiscard]] auto CanSignalDescription::multiplex_state() const -> MultiplexState { return m_multiplex_state; }
    void CanSignalDescription::set_multiplex_state(MultiplexState state) { m_multiplex_state = state; }

    [[nodiscard]] auto CanSignalDescription::comment() const -> std::string { return m_comment; }
    void CanSignalDescription::set_comment(std::string&& comment) { m_comment = std::move(comment); }
    void CanSignalDescription::set_comment(std::string_view comment) { m_comment = comment; }

    [[nodiscard]] auto CanSignalDescription::is_valid() const -> bool {
        if (m_name.empty() ||
            !(m_bit_length > 0 && m_bit_length <= 512) ||
            (m_data_format == DataFormat::Float && m_bit_length != 32) ||
            (m_data_format == DataFormat::Double && m_bit_length != 64) ||
            (m_data_format == DataFormat::AsciiString && (m_bit_length % 8 != 0))) {

            return false;
        }
        return true;
    }

    /**
     * CanMessageDescription
     */

    [[nodiscard]] auto CanMessageDescription::name() const -> std::string { return m_name; }
    void CanMessageDescription::set_name(std::string&& name) { m_name = name; }
    void CanMessageDescription::set_name(std::string_view name) { m_name = name; }

    [[nodiscard]] auto CanMessageDescription::id() const -> uint32_t { return m_id; }
    void CanMessageDescription::set_id(uint32_t id) { m_id = id; }

    [[nodiscard]] auto CanMessageDescription::length() const -> uint8_t { return m_length; }
    void CanMessageDescription::set_length(uint8_t length) { m_length = length; }

    [[nodiscard]] auto CanMessageDescription::transmitter() const -> std::string { return m_transmitter; }
    void CanMessageDescription::set_transmitter(std::string&& transmitter) { m_transmitter = transmitter; }
    void CanMessageDescription::set_transmitter(std::string_view transmitter) { m_transmitter = std::string(transmitter); }

    [[nodiscard]] auto CanMessageDescription::signal_descriptions() const -> std::vector<CanSignalDescription> { return m_signals; }

    auto CanMessageDescription::signal_description(std::string_view name) -> CanSignalDescription* {
        auto it = std::find_if(m_signals.begin(), m_signals.end(), [&name](auto const& s) {
            return s.m_name == name;
        });
        return it != m_signals.end() ? &(*it) : nullptr;
    }
    auto CanMessageDescription::signal_description(std::string_view name) const -> CanSignalDescription const* {
        auto it = std::find_if(m_signals.begin(), m_signals.end(), [&name](auto const& s) {
            return s.m_name == name;
        });
        return it != m_signals.end() ? &(*it) : nullptr;
    }

    void CanMessageDescription::clear_signal_descriptions() {
        m_signals.clear();
    }

    void CanMessageDescription::add_signal_description(CanSignalDescription signal) {
        m_signals.push_back(std::move(signal));
    }

    [[nodiscard]] auto CanMessageDescription::comment() const -> std::string { return m_comment; }
    void CanMessageDescription::set_comment(std::string&& comment) { m_comment = std::move(comment); }
    void CanMessageDescription::set_comment(std::string_view comment) { m_comment = comment; }

    [[nodiscard]] auto CanMessageDescription::is_valid() const -> bool {
        if (m_name.empty() ||
            m_signals.empty() ||
            std::ranges::none_of(m_signals, [](auto const& s) {
                return s.is_valid();
            })) {
            return false;
        }

        uint16_t total_signal_bits = 0;
        for (auto const& signal: m_signals) {
            total_signal_bits += signal.bit_length();
        }
        if (total_signal_bits > m_length * 8) {
            return false;
        }

        return true;
    }

} // namespace mrover::dbc
