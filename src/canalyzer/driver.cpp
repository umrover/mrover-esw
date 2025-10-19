#include "logger.cpp"


int main() {
    std::vector<Logger> loggers;
    logger_factory(loggers, "logger_start.yaml");
    for (size_t i = 0; i < loggers.size(); ++i) {
        loggers[i].print(std::cout);
    }
}