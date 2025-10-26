#include "logger.hpp"
#include <thread>

int main() {
    std::vector<logger::Logger> loggers;
    logger::logger_factory(loggers, "./logger_start.yaml");
    for (size_t i = 0; i < loggers.size(); ++i) {
        loggers[i].print(std::cout);
    }

    std::vector<std::thread> threads;
    threads.reserve(loggers.size());
    for (size_t i = 0; i < loggers.size(); ++i) {
        threads.emplace_back(&logger::Logger::start, &loggers[i]);
    }
    while (true) {
        sleep(1);   //may be not needed
    }
}