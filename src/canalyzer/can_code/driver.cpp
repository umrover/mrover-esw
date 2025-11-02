#include "logger.hpp"
#include <thread>

int main() {
    std::vector<logger::Logger> loggers;
    std::cout << "before factory\n";
    logger::logger_factory(loggers, "./logger_start.yaml", true);
    std::cout << "after factory\n";
    for (size_t i = 0; i < loggers.size(); ++i) {
        loggers[i].print(std::cout);
    }

    std::vector<std::thread> threads;
    threads.reserve(loggers.size());
    for (size_t i = 0; i < loggers.size(); ++i) {
    threads.emplace_back([&, i]() {
        try {
            std::cerr << "[Thread " << i << "] starting";
            loggers[i].start();
        } catch (const std::exception &e) {
            std::cerr << "[Thread " << i << "] crashed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[Thread " << i << "] crashed: unknown error" << std::endl;
        }
    });
}
    while (true) {
        sleep(1);   //may be not needed
    }
}