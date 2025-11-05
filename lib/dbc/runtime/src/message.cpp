#include "message.hpp"

#include <algorithm>

namespace mrover::dbc {

    /**
     * CanSignalDescription
     */

    [[nodiscard]] auto CanSignalDescription::get_name() const -> std::string { return m_name; }
    void CanSignalDescription::set_name(std::string const& name) { m_name = name; }
    void CanSignalDescription::set_name(std::string_view name) { m_name = std::string(name); }

    [[nodiscard]] auto CanSignalDescription::get_bit_start() const -> uint16_t { return m_bit_start; }
    void CanSignalDescription::set_bit_start(uint16_t bit) { m_bit_start = bit; }

    [[nodiscard]] auto CanSignalDescription::get_bit_length() const -> uint16_t { return m_bit_length; }
    void CanSignalDescription::set_bit_length(uint16_t length) { m_bit_length = length; }

    [[nodiscard]] auto CanSignalDescription::get_endianness() const -> Endianness { return m_endianness; }
    void CanSignalDescription::set_endianness(Endianness endianness) { m_endianness = endianness; }

    [[nodiscard]] auto CanSignalDescription::get_data_format() const -> DataFormat { return m_data_format; }
    void CanSignalDescription::set_data_format(DataFormat format) { m_data_format = format; }

    [[nodiscard]] auto CanSignalDescription::get_factor() const -> double { return m_factor; }
    void CanSignalDescription::set_factor(double factor) { m_factor = factor; }

    [[nodiscard]] auto CanSignalDescription::get_offset() const -> double { return m_offset; }
    void CanSignalDescription::set_offset(double offset) { m_offset = offset; }

    [[nodiscard]] auto CanSignalDescription::get_minimum() const -> double { return m_minimum; }
    void CanSignalDescription::set_minimum(double minimum) { m_minimum = minimum; }

    [[nodiscard]] auto CanSignalDescription::get_maximum() const -> double { return m_maximum; }
    void CanSignalDescription::set_maximum(double maximum) { m_maximum = maximum; }

    [[nodiscard]] auto CanSignalDescription::get_unit() const -> std::string { return m_unit; }
    void CanSignalDescription::set_unit(std::string const& unit) { m_unit = unit; }
    void CanSignalDescription::set_unit(std::string_view unit) { m_unit = std::string(unit); }

    [[nodiscard]] auto CanSignalDescription::get_receiver() const -> std::string { return m_receiver; }
    void CanSignalDescription::set_receiver(std::string const& receiver) { m_receiver = receiver; }
    void CanSignalDescription::set_receiver(std::string_view receiver) { m_receiver = std::string(receiver); }

    [[nodiscard]] auto CanSignalDescription::get_multiplex_state() const -> MultiplexState { return m_multiplex_state; }
    void CanSignalDescription::set_multiplex_state(MultiplexState state) { m_multiplex_state = state; }

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

    [[nodiscard]] auto CanMessageDescription::get_name() const -> std::string { return m_name; }
    void CanMessageDescription::set_name(std::string const& name) { m_name = name; }
    void CanMessageDescription::set_name(std::string_view name) { m_name = std::string(name); }

    [[nodiscard]] auto CanMessageDescription::get_id() const -> uint32_t { return m_id; }
    void CanMessageDescription::set_id(uint32_t id) { m_id = id; }

    [[nodiscard]] auto CanMessageDescription::get_length() const -> uint8_t { return m_length; }
    void CanMessageDescription::set_length(uint8_t length) { m_length = length; }

    [[nodiscard]] auto CanMessageDescription::get_transmitter() const -> std::string { return m_transmitter; }
    void CanMessageDescription::set_transmitter(std::string const& transmitter) { m_transmitter = transmitter; }
    void CanMessageDescription::set_transmitter(std::string_view transmitter) { m_transmitter = std::string(transmitter); }

    [[nodiscard]] auto CanMessageDescription::get_signal_descriptions() const -> std::vector<CanSignalDescription> { return m_signals; }
    [[nodiscard]] auto CanMessageDescription::get_signal_description_by_name(std::string const& name) const -> CanSignalDescription {
        return *std::find_if(m_signals.begin(), m_signals.end(), [&name](auto const& s) {
            return s.m_name == name;
        });
    }
    void CanMessageDescription::clear_signal_descriptions() {
        m_signals.clear();
    }

    void CanMessageDescription::add_signal_description(CanSignalDescription&& signal) {
        m_signals.push_back(std::move(signal));
    }

    void CanMessageDescription::add_signal_description(CanSignalDescription signal) {
        m_signals.push_back(std::move(signal));
    }

    [[nodiscard]] auto CanMessageDescription::is_valid() const -> bool {
        if (m_name.empty() ||
            m_signals.empty() ||
            std::ranges::none_of(m_signals, [](auto const& s) {
                return s.is_valid();
            })) {
            return false;
        }
        return true;
    }

} // namespace mrover::dbc
