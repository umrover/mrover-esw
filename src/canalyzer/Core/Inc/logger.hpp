#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dbc_runtime.hpp"
#include "influxdb.hpp"

namespace logger {

    auto make_can_timestamp() -> std::string;
    auto now_ms() -> long long;

    // GLOBAL
    inline std::atomic<bool> running;
    inline std::mutex cout_mutex;

    enum class log_mode {
        WHITELIST_IDS,
        BLACKLIST_IDS,
    };

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
                      long long timestamp);
            auto commit(influxdb_cpp::server_info const& si) -> int;
        };

        int id;
        int bus_socket = -1;
        std::string can_bus_name;
        std::string yaml_file_path;
        std::string dbc_root_path;
        influxdb_cpp::server_info si;

        std::unordered_set<uint32_t> log_ids;
        std::unordered_set<std::string> dbc_file_paths;

        mrover::dbc_runtime::CanDbcFileParser parser;
        mrover::dbc_runtime::CanFrameProcessor processor;

        std::deque<DecodedFrame> buffer;
        std::mutex buffer_mutex;
        std::condition_variable cv;

        void _committer_worker();
        std::thread committer_thread;

        DynamicBuilder builder;

        log_mode mode;
        bool debug = false;
        bool done = false;
        bool log_ascii;

        int read_error_count = 0;            //read error on the can bus
        int read_error_count_incomplete = 0; //if size is less than frame.
        int influx_post_error_count = 0;

        void _init_bus();
        void _log_ascii(
                unsigned char const* arr,
                std::string const& name,
                std::ofstream& outputFile,
                uint32_t id);

        auto _decode(uint32_t id, canfd_frame const& can_frame) -> std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue>;
        void _stop_bus();

    public:
        Logger(
                int id,
                std::string& bus_name,
                std::string& yaml_file_path,
                std::unordered_set<uint32_t>&& log_ids,
                std::unordered_set<std::string>&& dbc_file_paths,
                influxdb_cpp::server_info& si,
                log_mode mode,
                bool log_ascii);

        Logger(Logger&& other) noexcept;
        void start();

        void print() const;
        void print_error() const;

        Logger(Logger const&) = delete;
        auto operator=(Logger const&) -> Logger& = delete;
    };

    auto logger_factory(std::string& yaml_path) -> std::vector<Logger>;
    static auto trim(std::string const& s) -> std::string;

    void handle_SIGINT(int);
    void run_bus(std::vector<Logger>& loggers);
} // namespace logger
