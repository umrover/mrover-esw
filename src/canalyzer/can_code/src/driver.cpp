#include "logger.hpp"

int main(int argc, char* argv[]) {
    std::string path = "../logger_start.yaml";
    std::vector<logger::Logger> loggers = logger::logger_factory(path, true);
    run_bus(loggers);
}
