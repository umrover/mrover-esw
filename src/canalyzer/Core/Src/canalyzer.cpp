#include "logger.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage: ./canalyzer <path_to_yaml>";
        return 1;
    }
    logger::running.store(true);
    std::string yaml_path(argv[1]);
    std::vector<logger::Logger> loggers = logger::logger_factory(yaml_path);
    run_bus(loggers);
}
