#include "logger.hpp"
#include <thread>

int main() {
    std::vector<std::unique_ptr<logger::Logger>> loggers;
    std::cout << "before factory\n";
    try {
        logger::logger_factory(loggers, "./logger_start.yaml", true);

        if (loggers.empty()) {
            std::cerr << "no loggers were configured \n";
            return 1;
        }

    } catch (const std::exception &e) {
        std::cerr << "logger factory broke :(, " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "after factory\n";

    std::vector<std::thread> threads;
    threads.reserve(loggers.size());
    for (size_t i = 0; i < loggers.size(); ++i) {
    threads.emplace_back([&, i]() {
        try {
            std::cerr << "[Thread " << i << "] starting";
            loggers[i]->start();
        } catch (const std::exception &e) {
            std::cerr << "[Thread " << i << "] crashed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[Thread " << i << "] crashed: unknown error" << std::endl;
        }
    });
}
    for (int i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
}