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

    static constexpr auto MESSAGE_HEADER = "BO_ ";
    static constexpr auto SIGNAL_HEADER = "SG_ ";
    static constexpr auto SIGNAL_TYPE_HEADER = "SIG_VAL_TYPE_ ";
    static constexpr auto COMMENT_HEADER = "CM_ ";
    static constexpr auto SYMBOLS_HEADER = "NS_ ";

    namespace {
        constexpr inline auto trimmed(std::string_view str) -> std::string_view {
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

        template<std::floating_point T>
        [[nodiscard]] auto to_floating_point(string_view sv) noexcept -> std::expected<T, std::errc> {
            T value{};
            char const* first = sv.data();
            char const* last = sv.data() + sv.length();

            std::from_chars_result res = std::from_chars(first, last, value);

            if (res.ec != std::errc()) return std::unexpected(res.ec);
            if (res.ptr != last) return std::unexpected(std::errc::invalid_argument);

            return std::expected<T, std::errc>(std::in_place, value);
        }

    } // namespace

    [[nodiscard]] auto CanDbcFileParser::lines_parsed() const -> std::size_t { return m_lines_parsed; }

    [[nodiscard]] auto CanDbcFileParser::is_error() const -> bool { return m_error != CanDbcFileParser::Error::None; }
    [[nodiscard]] auto CanDbcFileParser::error() const -> Error { return m_error; }

    [[nodiscard]] auto CanDbcFileParser::messages() const -> std::unordered_map<uint32_t, CanMessageDescription> const& { return m_messages; }

    auto CanDbcFileParser::parse(std::string const& filename) -> bool {
        std::ifstream file(filename);
        if (!file.is_open()) {
            m_error = CanDbcFileParser::Error::FileRead;
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
            m_error = CanDbcFileParser::Error::FileRead;
            return false;
        }

        if (file.fail() && !file.eof()) {
            m_error = CanDbcFileParser::Error::FileRead;
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
                    m_error = CanDbcFileParser::Error::InvalidMessageFormat;
                    return false;
                }
            }
            m_is_processing_message = true;

            auto message = parse_message(line);
            if (!message.has_value()) {
                m_error = message.error();
                return false;
            }
            m_current_message = std::move(message.value());

            return true;

        } else if (line.starts_with(SIGNAL_HEADER)) {
            if (!m_is_processing_message) {
                m_error = CanDbcFileParser::Error::InvalidSignalFormat;
                return false;
            }

            auto signal = parse_signal(line);
            if (!signal.has_value()) {
                m_error = signal.error();
                return false;
            }
            m_current_message.add_signal_description(signal.value());

            return true;
        } else if (line.starts_with(COMMENT_HEADER)) {
            auto comment_info = parse_comment(line);
            if (!comment_info.has_value()) {
                m_error = comment_info.error();
                return false;
            }

            uint32_t message_id = std::get<0>(comment_info.value());
            CanMessageDescription* message_desc = nullptr;

            if (auto message_it = m_messages.find(message_id); message_it != m_messages.end()) {
                message_desc = &message_it->second;
            } else if (m_is_processing_message && m_current_message.id() == message_id) {
                message_desc = &m_current_message;
            }

            if (message_desc == nullptr) {
                m_error = CanDbcFileParser::Error::InvalidCommentMessageId;
                return false;
            }

            auto& signal_comment = std::get<1>(comment_info.value());
            std::string_view comment = std::get<2>(comment_info.value());

            if (signal_comment.has_value()) {
                auto const& signal_name = signal_comment.value();
                auto* signal_desc = message_desc->signal_description(signal_name);
                if (signal_desc == nullptr) {
                    m_error = CanDbcFileParser::Error::InvalidCommentSignalName;
                    return false;
                }
                signal_desc->set_comment(comment);
            } else {
                message_desc->set_comment(comment);
            }

            return true;
        }

        return true;
    }

    auto CanDbcFileParser::parse_message(string_view line) -> std::expected<CanMessageDescription, Error> {
        if (!line.starts_with(MESSAGE_HEADER)) {
            return std::unexpected(Error::InvalidMessageFormat);
        }

        line.remove_prefix(std::string_view(MESSAGE_HEADER).size());

        CanMessageDescription message;
        // ===== ID =====
        string_view id = next_word(line);
        auto id_result = to_int<uint32_t>(id);
        if (!id_result.has_value()) {
            return std::unexpected(Error::InvalidMessageId);
        }
        message.set_id(id_result.value());

        // ===== NAME =====
        string_view name = next_word(line);
        if (!name.ends_with(':')) {
            return std::unexpected(Error::InvalidMessageName);
        }
        name.remove_suffix(1);
        message.set_name(name);

        // ===== LENGTH =====
        string_view length = next_word(line);
        auto length_result = to_int<uint8_t>(length);
        if (!length_result.has_value()) {
            return std::unexpected(Error::InvalidMessageLength);
        }
        message.set_length(length_result.value());

        // ===== TRANSMITTER =====
        string_view transmitter = next_word(line);
        if (transmitter.empty()) {
            return std::unexpected(Error::InvalidMessageTransmitter);
        }
        message.set_transmitter(transmitter);

        if (!line.empty()) {
            return std::unexpected(Error::InvalidMessageFormat);
        }

        return std::expected<CanMessageDescription, Error>(std::in_place, message);
    }

    auto CanDbcFileParser::parse_signal(string_view line) -> std::expected<CanSignalDescription, Error> {
        if (!line.starts_with(SIGNAL_HEADER)) {
            return std::unexpected(Error::InvalidSignalFormat);
        }

        line.remove_prefix(std::string_view(SIGNAL_HEADER).size());

        // ===== SIGNAL NAME =====
        CanSignalDescription signal;
        string_view name = next_word(line);
        if (name.empty()) {
            return std::unexpected(Error::InvalidSignalName);
        }
        signal.set_name(name);

        // ===== MULTIPLEX =====
        string_view colon_or_multiplex = next_word(line);
        if (colon_or_multiplex == ":") {
            signal.set_multiplex_state(MultiplexState::None);
        } else if (colon_or_multiplex == "M") {
            signal.set_multiplex_state(MultiplexState::MultiplexorSwitch);
            string_view colon = next_word(line);
            if (colon != ":") {
                return std::unexpected(Error::InvalidSignalFormat);
            }
        } else if (colon_or_multiplex.starts_with("m")) {
            signal.set_multiplex_state(MultiplexState::MultiplexedSignal);
            string_view colon = next_word(line);
            if (colon != ":") {
                return std::unexpected(Error::InvalidSignalFormat);
            }
        } else {
            return std::unexpected(Error::InvalidSignalFormat);
        }

        // ===== BIT INFO =====
        string_view bit_info = next_word(line);
        size_t sep_pos = bit_info.find('@');
        if (sep_pos == std::string_view::npos) {
            return std::unexpected(Error::InvalidSignalBitInfo);
        }
        string_view endianess_and_format = bit_info.substr(sep_pos + 1);
        if (endianess_and_format.size() != 2) {
            return std::unexpected(Error::InvalidSignalBitInfo);
        }
        char endianess_char = endianess_and_format[0];
        if (endianess_char == '0') {
            signal.set_endianness(Endianness::BigEndian);
        } else if (endianess_char == '1') {
            signal.set_endianness(Endianness::LittleEndian);
        } else {
            return std::unexpected(Error::InvalidSignalBitInfo);
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
            return std::unexpected(Error::InvalidSignalBitInfo);
        }
        bit_start_str = bit_start_end.substr(0, sep_pos);
        bit_length_str = bit_start_end.substr(sep_pos + 1);
        auto bit_start_result = to_int<uint16_t>(bit_start_str);
        if (!bit_start_result.has_value()) {
            return std::unexpected(Error::InvalidSignalBitInfo);
        }
        signal.set_bit_start(bit_start_result.value());
        auto bit_length_result = to_int<uint16_t>(bit_length_str);
        if (!bit_length_result.has_value()) {
            return std::unexpected(Error::InvalidSignalBitInfo);
        }
        signal.set_bit_length(bit_length_result.value());

        // ===== FACTOR & OFFSET =====
        string_view factor_and_offset = next_word(line);
        if (!factor_and_offset.starts_with('(') || !factor_and_offset.ends_with(')')) {
            return std::unexpected(Error::InvalidSignalFactorOffset);
        }
        factor_and_offset.remove_prefix(1);
        factor_and_offset.remove_suffix(1);
        sep_pos = factor_and_offset.find(',');
        if (sep_pos == std::string_view::npos) {
            return std::unexpected(Error::InvalidSignalFactorOffset);
        }
        string_view factor_str = factor_and_offset.substr(0, sep_pos);
        string_view offset_str = factor_and_offset.substr(sep_pos + 1);
        auto factor = to_floating_point<double>(factor_str);
        if (!factor.has_value()) {
            return std::unexpected(Error::InvalidSignalFactorOffset);
        }
        signal.set_factor(factor.value());
        auto offset = to_floating_point<double>(offset_str);
        if (!offset.has_value()) {
            return std::unexpected(Error::InvalidSignalFactorOffset);
        }
        signal.set_offset(offset.value());

        // ===== MIN & MAX =====
        string_view min_and_max = next_word(line);
        if (!min_and_max.starts_with('[') || !min_and_max.ends_with(']')) {
            return std::unexpected(Error::InvalidSignalMinMax);
        }
        min_and_max.remove_prefix(1);
        min_and_max.remove_suffix(1);
        sep_pos = min_and_max.find('|');
        if (sep_pos == std::string_view::npos) {
            return std::unexpected(Error::InvalidSignalMinMax);
        }
        string_view min_str = min_and_max.substr(0, sep_pos);
        string_view max_str = min_and_max.substr(sep_pos + 1);
        auto min = to_floating_point<double>(min_str);
        if (!min.has_value()) {
            return std::unexpected(Error::InvalidSignalMinMax);
        }
        signal.set_minimum(min.value());
        auto max = to_floating_point<double>(max_str);
        if (!max.has_value()) {
            return std::unexpected(Error::InvalidSignalMinMax);
        }
        signal.set_maximum(max.value());

        // ===== UNIT =====
        string_view unit = next_word(line);
        if (unit.front() != '"' || unit.back() != '"') {
            return std::unexpected(Error::InvalidSignalUnit);
        }
        unit.remove_prefix(1);
        unit.remove_suffix(1);
        signal.set_unit(unit);

        // ===== RECEIVER =====
        string_view receiver = next_word(line);
        if (!receiver.empty()) {
            signal.set_receiver(receiver);
        }

        if (!line.empty() || !signal.is_valid()) {
            return std::unexpected(Error::InvalidSignalFormat);
        }

        return std::expected<CanSignalDescription, Error>(std::in_place, signal);
    }


    auto CanDbcFileParser::parse_comment(std::string_view line) -> std::expected<std::tuple<uint32_t, std::optional<std::string>, std::string>, Error> {
        if (!line.starts_with(COMMENT_HEADER)) {
            return std::unexpected(Error::InvalidCommentFormat);
        }

        line.remove_prefix(std::string_view(COMMENT_HEADER).size());

        string_view target_type = next_word(line);
        if (target_type != "BO_" && target_type != "SG_") {
            return std::unexpected(Error::InvalidCommentType);
        }

        // ===== ID =====
        string_view id_str = next_word(line);
        auto id_result = to_int<uint32_t>(id_str);
        if (!id_result.has_value()) {
            return std::unexpected(Error::InvalidCommentMessageId);
        }
        uint32_t id = id_result.value();

        std::optional<std::string> signal_name;
        if (target_type == "SG_") {
            string_view signal_name_str = next_word(line);
            if (signal_name_str.empty()) {
                return std::unexpected(Error::InvalidCommentSignalName);
            }
            signal_name = std::string(signal_name_str);
        }

        // ===== COMMENT =====
        string_view comment_str = trimmed(line);
        if (comment_str.front() != '"') {
            return std::unexpected(Error::InvalidCommentText);
        }
        comment_str.remove_prefix(1);

        if (comment_str.back() == ';') {
            comment_str.remove_suffix(1);
            comment_str = trimmed(comment_str);
        }

        if (comment_str.back() != '"') {
            return std::unexpected(Error::InvalidCommentText);
        }
        comment_str.remove_suffix(1);

        return std::expected<std::tuple<uint32_t, std::optional<std::string>, std::string>, Error>(std::in_place, id, signal_name, comment_str);
    }

    auto CanDbcFileParser::add_current_message() -> bool {
        if (m_is_processing_message) {
            if (!m_current_message.is_valid()) {
                return false;
            }
            uint32_t const id = m_current_message.id();
            m_messages.emplace(id, std::move(m_current_message));
            m_current_message = {};
            return true;
        }
        return false;
    }


} // namespace mrover::dbc
