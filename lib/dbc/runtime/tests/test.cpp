#include "dbc.hpp"

#include <iostream>

auto main(int argc, char** argv) -> int {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dbc_file>" << std::endl;
        return 1;
    }
    std::string dbc_filename = argv[1];
    mrover::dbc::CanDbcFileParser parser;
    if (!parser.parse(dbc_filename)) {
        std::cerr << "Failed to parse DBC file: " << dbc_filename << std ::endl;
        std::cerr << "Error after parsing " << parser.get_lines_parsed() << " lines." << std::endl;
        if (parser.is_error()) {
            std::cerr << "Error: " << parser.get_error() << std::endl;
        }
        return 1;
    }

    for (auto const& [id, message] : parser.get_messages()) {
        std::cout << message << std::endl;
    }

    return 0;
}
