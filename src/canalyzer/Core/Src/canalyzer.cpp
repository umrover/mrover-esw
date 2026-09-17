#include "logger.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "usage: ./canalyzer <path_to_yaml> <path_to_dbc_folder>";
        return 1;
    }
    logger::running.store(true);
    std::string yaml_path(argv[1]);
    std::string dbc_path(argv[2]);
    std::vector<logger::Logger> loggers = logger::logger_factory(yaml_path, dbc_path);
    run_bus(loggers);
}
