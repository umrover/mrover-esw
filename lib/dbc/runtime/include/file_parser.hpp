#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "message.hpp"

/*
    The current implementation supports only a subset of keywords that you can find in a DBC file:
        {BO_} - message description.
        {SG_} - signal description.
        {SIG_VALTYPE_} - signal type description.
        {SG_MUL_VAL_} - extended multiplexing description.
        {CM_} - comments (only for message and signal descriptions).
        {VAL_} - textual descriptions for raw signal values.
*/

namespace mrover::dbc {
    class CanDbcFileParser {
    public:
        enum class Error {
            None,
            InvalidMessage, 
            InvalidSignal
        };

        CanDbcFileParser() = default;

        [[nodiscard]] auto get_lines_parsed() const -> std::size_t;

        [[nodiscard]] auto is_error() const -> bool;
        [[nodiscard]] auto get_error() const -> Error;

        auto parse(std::string const& filename) -> bool;

    private:
        std::unordered_map<uint32_t, CanMessageDescription> m_messages{};
        CanMessageDescription m_current_message;
        bool m_is_processing_message = false;
        std::size_t m_lines_parsed = 0;
        Error m_error = Error::None;

        auto process_line(std::string_view line) -> bool;
        auto parse_message(std::string_view line) -> bool;
        auto parse_signal(std::string_view line) -> bool;
        auto add_current_message() -> bool;
    };
} // namespace mrover::dbc
