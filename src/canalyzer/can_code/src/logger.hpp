#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <memory>
#include <queue>
#include <chrono>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "influxdb.hpp"
#include "Yaml.hpp"

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

    Auth(Auth &&other) noexcept = default;  
    Auth(std::string host, int port, std::string database, std::string user, std::string password);
};

class Logger {
    private:
        int bus_socket;
        std::string can_bus_name;
        std::string yaml_file_path;
        std::string ascii_log_file_path;
        influxdb_cpp::server_info si;
        //std::unordered_set<int> valid_ids;
        std::unordered_set<int> log_ids;
        std::unordered_set<std::string> dbc_file_paths;
        std::queue<canfd_frame> buffer;
        bool log_all = false;
        bool debug = false;

        int read_error_count = 0;                   //read error on the can bus
        int read_error_count_incomplete = 0;        //if size is less than frame.
        int influx_post_error_count = 0;

        void _init_bus();
        void _log_ascii(unsigned char *arr, std::string name, std::ofstream &outputFile);
    
    public:
        Logger(std::string bus_name, std::string yaml_file_path, std::string ascii_log_file_path, Auth &server_info, 
            std::unordered_set<int> &&log_ids, std::unordered_set<std::string> &&dbc_file_paths, bool log_all, bool debug/*, std::istream &is*/);
        void start();
        void print(std::ostream &os);
        friend void test_factory(std::vector<std::unique_ptr<Logger>> loggers);

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

};

void logger_factory(std::vector<std::unique_ptr<Logger>> &loggers, std::string yaml_path, bool debug = false);
void test_factory(const std::vector<std::unique_ptr<Logger>>& loggers);
}