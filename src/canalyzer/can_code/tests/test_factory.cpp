#include <cassert>

#include "logger.hpp"

namespace logger {

    void test_factory(std::vector<Logger>& loggers) {
        assert(loggers[0].can_bus_name == "vcan0");
        assert(loggers[0].log_ids.count(100));
        assert(loggers[0].log_ids.count(101));
        assert(loggers[0].log_all == false);
        assert(loggers[0].ascii_log_file_path == "./log/can0.log");
        assert(loggers[0].dbc_file_paths.count("./dbc/can0.dbc"));

        assert(loggers[1].can_bus_name == "vcan1");
        assert(loggers[1].log_ids.count(1));
        assert(loggers[1].log_ids.count(2));
        assert(loggers[1].log_ids.count(3));
        assert(loggers[1].log_all == false);
        assert(loggers[1].ascii_log_file_path == "./log/can1.log");
        assert(loggers[1].dbc_file_paths.count("./dbc/can1.dbc"));

        assert(loggers[2].can_bus_name == "vcan2");
        assert(loggers[2].log_ids.count(98));
        assert(loggers[2].log_ids.count(123));
        assert(loggers[2].log_all == false);
        assert(loggers[2].ascii_log_file_path == "./log/can2.log");
        assert(loggers[2].dbc_file_paths.count("./dbc/can2.dbc"));

        assert(loggers[3].can_bus_name == "vcan3");
        assert(loggers[3].log_ids.count(21));
        assert(loggers[3].log_ids.count(2010339));
        assert(loggers[3].log_all == false);
        assert(loggers[3].dbc_file_paths.count("./dbc/can3.dbc"));
        assert(loggers[3].dbc_file_paths.count("./dbc/can4.dbc"));
        assert(loggers[3].ascii_log_file_path == "./log/can3.log");

        std::cout << "Test Passing!" << std::endl;
    }
} // namespace logger

int main() {
    std::string file_name = "../logger_start.yaml";

    std::vector<logger::Logger> loggers = logger::logger_factory(file_name);

    assert(loggers.size() == 4);

    test_factory(loggers);
    return 1;
}
