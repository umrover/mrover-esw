#include "logger.cpp"
#include <thread>

int main() {
    std::vector<Logger> loggers;
    logger_factory(loggers, "logger_start.yaml");
    for (size_t i = 0; i < loggers.size(); ++i) {
        loggers[i].print(std::cout);
    }

    std::vector<std::thread> threads;
    threads.reserve(loggers.size());
    for (size_t i = 0; i < loggers.size(); ++i) {
        threads.emplace_back(&Logger::start, &loggers[i]);
    }
    while (true) {
        sleep(1);
    }
}