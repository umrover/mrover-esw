#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

namespace mrover {

    inline auto debug(std::string const& input) -> void {
        printf("%s\n\r", input.c_str());
    }

    inline auto debug(char const* input) -> void {
        printf("%s\n\r", input);
    }

    inline auto debugf(char const* format, ...) -> void {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

} // namespace mrover
