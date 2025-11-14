#pragma once

#include <string>
#include <cstdio>
#include <cstdarg>

namespace mrover {

    inline auto debug(const std::string& input) -> void {
        printf("%s\n\r", input.c_str());
    }

    inline auto debug(const char* input) -> void {
        printf("%s\n\r", input);
    }

    inline auto debugf(const char* format, ...) -> void {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

} // namespace mrover