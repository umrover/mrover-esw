#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include "influxdb.hpp"
#include <linux/can.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <queue>
#include <chrono>
#include "Yaml.hpp"
#include <iomanip>

#include <sys/stat.h>   // for mkdir
#include <sys/types.h>
#include <filesystem>   

namespace logger {

std::string decode(canid_t can_id);
long long now_ns();

struct Auth {
    std::string host;
    int port;
    std::string db_name;
    std::string user;
    std::string password;
};

class Logger {
    private:
        int bus_socket;
        std::string can_bus_name;
        std::string file_path;
        influxdb_cpp::server_info si;
        //std::unordered_set<int> valid_ids;
        std::unordered_set<int> listen_ids;
        std::queue<canfd_frame> buffer;
        bool log_all = false;
        bool debug = false;

        void _init_bus();
        void _log_ascii(unsigned char *arr, std::string name, std::ofstream &outputFile);
    
    public:
        Logger(std::string bus_name, Auth &server_info, std::unordered_set<int> can_ids_listen, bool log_all, std::string file_path, bool debug/*, std::istream &is*/);
        void start();
        void print(std::ostream &os);
};

void logger_factory(std::vector<Logger> &loggers, std::string path, bool debug = false);

}