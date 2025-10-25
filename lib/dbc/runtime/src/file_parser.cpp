#include "file_parser.hpp"

#include <charconv>
#include <concepts>
#include <expected>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace mrover::dbc {
    using std::string_view;

    static constexpr auto MESSAGE_HEADER = "BO_";
    static constexpr auto SIGNAL_HEADER = "SG_";
    static constexpr auto SIGNAL_TYPE_HEADER = "SIG_VAL_TYPE_";

    namespace {
        inline auto trimmed(std::string_view str) -> std::string_view {
            auto const is_space = [](unsigned char c) { return std::isspace(c); };

            auto start = str.find_first_not_of(" \t\n\r\f\v");
            if (start == std::string_view::npos) {
                return {}; // all whitespace
            }

            auto end = str.find_last_not_of(" \t\n\r\f\v");
            return str.substr(start, end - start + 1);
        }

        inline void strip_utf8_bom_if_present(std::string& s) noexcept {
            if (s.size() >= 3 &&
                static_cast<unsigned char>(s[0]) == 0xEF &&
                static_cast<unsigned char>(s[1]) == 0xBB &&
                static_cast<unsigned char>(s[2]) == 0xBF) {
                s.erase(0, 3);
            }
        }

        auto next_word(string_view& sv) -> string_view {
            size_t start = 0;
            while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start]))) {
                ++start;
            }
            sv.remove_prefix(start);

            if (sv.empty()) {
                return {};
            }

            size_t end = 0;
            while (end < sv.size() && !std::isspace(static_cast<unsigned char>(sv[end]))) {
                ++end;
            }

            string_view word = sv.substr(0, end);

            sv.remove_prefix(end);

            size_t next_start = 0;
            while (next_start < sv.size() && std::isspace(static_cast<unsigned char>(sv[next_start]))) {
                ++next_start;
            }
            sv.remove_prefix(next_start);

            return word;
        }

        template<std::integral T>
            requires(!std::same_as<T, bool>)
        [[nodiscard]] auto to_int(string_view sv, int base = 10) noexcept -> std::expected<T, std::errc> {
            if (base < 2 || base > 36) {
                return std::unexpected(std::errc::invalid_argument);
            }

            T value{};
            char const* first = sv.data();
            char const* last = sv.data() + sv.length();

            std::from_chars_result res = std::from_chars(first, last, value, base);

            if (res.ec != std::errc()) return std::unexpected(res.ec);
            if (res.ptr != last) return std::unexpected(std::errc::invalid_argument);

            return std::expected<T, std::errc>(std::in_place, value);
        }

    } // namespace

    [[nodiscard]] auto CanDbcFileParser::get_lines_parsed() const -> std::size_t { return m_lines_parsed; }

    [[nodiscard]] auto CanDbcFileParser::is_error() const -> bool { return m_error != CanDbcFileParser::Error::None; }
    [[nodiscard]] auto CanDbcFileParser::get_error() const -> Error { return m_error; }

    auto CanDbcFileParser::parse(std::string const& filename) -> bool {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        m_lines_parsed = 0;
        while (std::getline(file, line)) {
            if (m_lines_parsed == 0) {
                strip_utf8_bom_if_present(line);
            }

            if (!process_line(line)) {
                return false;
            }

            ++m_lines_parsed;
        }

        if (file.bad()) {
            return false;
        }

        if (file.fail() && !file.eof()) {
            return false;
        }

        if (m_is_processing_message) {
            if (!add_current_message()) {
                return false;
            }
        }

        return true;
    }

    auto CanDbcFileParser::process_line(string_view line) -> bool {
        line = trimmed(line);

        if (line.empty() || line.starts_with("//")) {
            return true;
        } else if (line.starts_with(MESSAGE_HEADER)) {
            if (m_is_processing_message) {
                if (!add_current_message()) {
                    m_error = CanDbcFileParser::Error::InvalidMessage;
                    return false;
                }
            }
            m_is_processing_message = true;

            if (!parse_message(line)) {
                m_error = CanDbcFileParser::Error::InvalidMessage;
                return false;
            }

            return true;

        } else if (line.starts_with(SIGNAL_HEADER)) {
            if (!m_is_processing_message) {
                m_error = CanDbcFileParser::Error::InvalidSignal;
                return false;
            }

            if (!parse_signal(line)) {
                m_error = CanDbcFileParser::Error::InvalidSignal;
                return false;
            }

            return true;
        }

        return false;
    }

    auto CanDbcFileParser::parse_message(string_view line) -> bool {
        if (!line.starts_with(MESSAGE_HEADER)) {
            return false;
        }

        line.remove_prefix(std::string_view(MESSAGE_HEADER).size());

        string_view id = next_word(line);
        auto id_result = to_int<uint32_t>(id);
        if (!id_result.has_value()) {
            return false;
        }
        m_current_message.set_id(id_result.value());

        string_view name = next_word(line);
        if (!name.ends_with(':')) {
            return false;
        }
        name.remove_suffix(1);
        m_current_message.set_name(name);

        string_view length = next_word(line);
        auto length_result = to_int<uint8_t>(length);
        if (!length_result.has_value()) {
        }
        m_current_message.set_length(length_result.value());

        string_view transmitter = next_word(line);
        if (transmitter.empty()) {
            return false;
        }
        m_current_message.set_transmitter(transmitter);

        if (!line.empty()) {
            return false;
        }

        return true;
    }

    auto CanDbcFileParser::parse_signal(string_view line) -> bool {
        if (!line.starts_with(SIGNAL_HEADER)) {
            return false;
        }

        line.remove_prefix(std::string_view(SIGNAL_HEADER).size());

        CanSignalDescription signal;
        string_view name = next_word(line);
        if (name.empty()) {
            return false;
        }

        signal.set_name(name);

        string_view colon_or_multiplex = next_word(line);
        if (colon_or_multiplex == ":") {
            signal.set_multiplex_state(MultiplexState::None);
        } else if (colon_or_multiplex == "M") {
            signal.set_multiplex_state(MultiplexState::MultiplexorSwitch);
            string_view colon = next_word(line);
            if (colon != ":") {
                return false;
            }
        } else if (colon_or_multiplex.starts_with("m")) {
            signal.set_multiplex_state(MultiplexState::MultiplexedSignal);
            string_view colon = next_word(line);
            if (colon != ":") {
                return false;
            }
        } else {
            return false;
        }

        string_view bit_info = next_word(line);
        size_t sep_pos = bit_info.find('@');
        if (sep_pos == std::string_view::npos) {
            return false;
        }
        string_view endianess_and_format = bit_info.substr(sep_pos + 1);
        if (endianess_and_format.size() != 2) {
            return false;
        }
        char endianess_char = endianess_and_format[0];
        if (endianess_char == '0') {
            signal.set_endianness(Endianness::BigEndian);
        } else if (endianess_char == '1') {
            signal.set_endianness(Endianness::LittleEndian);
        } else {
            return false;
        }
        char format_char = endianess_and_format[1];
        if (format_char == '+') {
            signal.set_data_format(DataFormat::UnsignedInteger);
        } else if (format_char == '-') {
            signal.set_data_format(DataFormat::SignedInteger);
        }


        string_view bit_start_end = bit_info.substr(0, sep_pos);
        string_view bit_start_str;
        string_view bit_length_str;
        sep_pos = bit_start_end.find('|');
        if (sep_pos == std::string_view::npos) {
            return false;
        }
        bit_start_str = bit_start_end.substr(0, sep_pos);
        bit_length_str = bit_start_end.substr(sep_pos + 1);
        auto bit_start_result = to_int<uint16_t>(bit_start_str);
        if (!bit_start_result.has_value()) {
            return false;
        }
        signal.set_bit_start(bit_start_result.value());
        auto bit_length_result = to_int<uint16_t>(bit_length_str);
        if (!bit_length_result.has_value()) {
            return false;
        }
        signal.set_bit_length(bit_length_result.value());


        return true;
    }

    auto CanDbcFileParser::add_current_message() -> bool {
        if (m_is_processing_message) {
            if (!m_current_message.is_valid()) {
                return false;
            }
            uint32_t const id = m_current_message.get_id();
            m_messages.emplace(id, std::move(m_current_message));
            m_current_message = {};
            return true;
        }
        return false;
    }


} // namespace mrover::dbc
