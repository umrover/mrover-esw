#include "logger.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage: ./canalyzer <path_to_yaml>";
        return 1;
    }
    std::string path(argv[1]);
    std::vector<logger::Logger> loggers = logger::logger_factory(path, true);
    run_bus(loggers);
}
