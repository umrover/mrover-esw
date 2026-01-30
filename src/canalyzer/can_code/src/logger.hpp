#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../../../lib/dbc/runtime/include/dbc_runtime.hpp" //we should fix this
#include "Yaml.hpp"
#include "influxdb.hpp"

namespace logger {

    std::string decode(canid_t can_id);
    std::string make_can_timestamp();
    long long now_ns();

    struct Auth {
        std::string host;
        int port;
        std::string db_name;
        std::string user;
        std::string password;
        Auth(Auth&& other) noexcept = default;
        Auth(std::string host, int port, std::string database, std::string user, std::string password);
    };

    // GLOBAL
    extern std::atomic<bool> running;
    extern std::mutex cout_mutex;

    class Logger {
    private:
        struct DecodedFrame {
            uint32_t id;
            long long time;
            std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue> data;
        };

        struct DynamicBuilder : public influxdb_cpp::builder {
            void post(std::string const& measurement,
                      std::string const& bus_name,
                      std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue> const& data,
                      long long const timestamp);
        };

        int id;
        int bus_socket;
        std::string can_bus_name;
        std::string yaml_file_path;
        std::string ascii_log_file_path;
        influxdb_cpp::server_info si;

        std::unordered_set<int> log_ids;
        std::unordered_set<std::string> dbc_file_paths;

        mrover::dbc_runtime::CanDbcFileParser parser;
        mrover::dbc_runtime::CanFrameProcessor processor;

        std::deque<DecodedFrame> buffer;
        std::mutex buffer_mutex;
        std::condition_variable cv;

        void _committer_worker();
        std::thread committer_thread;

        DynamicBuilder builder;

        bool log_all = false;
        bool debug = false;
        bool done = false;

        int read_error_count = 0;            //read error on the can bus
        int read_error_count_incomplete = 0; //if size is less than frame.
        int influx_post_error_count = 0;

        void _init_bus();
        void _log_ascii(
                unsigned char* arr,
                std::string name,
                std::ofstream& outputFile,
                uint32_t id);

        auto _decode(uint32_t const id, canfd_frame const& can_frame) -> std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue>;
        void _stop_bus();

    public:
        Logger(
                int id,
                std::string& bus_name,
                std::string& yaml_file_path,
                std::string& ascii_log_file_path,
                Auth& server_info,
                std::unordered_set<int>&& log_ids,
                std::unordered_set<std::string>&& dbc_file_paths,
                bool log_all,
                bool debug);

        Logger(Logger&& other) noexcept;
        void start();

        void print();
        void print_error();

        friend void test_factory(std::vector<Logger>& loggers);

        Logger(Logger const&) = delete;
        Logger& operator=(Logger const&) = delete;
    };

    std::vector<Logger> logger_factory(std::string& yaml_path, bool debug = false);
    void test_factory(std::vector<Logger> const& loggers);

    void handle_SIGINT(int);
    void run_bus(std::vector<Logger>& loggers);
} // namespace logger
