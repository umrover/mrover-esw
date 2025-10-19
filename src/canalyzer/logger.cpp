#include <string>
#include <iostream>
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

std::string decode(canid_t can_id);

long long now_ns() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}


struct Auth {
    std::string host;
    int port;
    std::string db_name;
    std::string user;
    std::string password;
};

class Logger {
    private:
        int bus_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        std::string can_bus_name;
        std::string file_path;
        influxdb_cpp::server_info si;
        //std::unordered_set<int> valid_ids;
        std::unordered_set<int> listen_ids;
        std::queue<canfd_frame> buffer;
        bool log_all = false;





        void init_bus() {
            struct ifreq ifr;
            struct sockaddr_can addr;
            const char *c_can_id = can_bus_name.c_str();
            strcpy(ifr.ifr_name, c_can_id);

            if (ioctl(bus_socket, SIOGIFINDEX, &ifr) < 0) {
                std::cerr << "ioctl broke" << std::endl;
                //blow up exit(1)?
            }
            addr.can_family = AF_CAN;
            addr.can_ifindex = ifr.ifr_ifindex;

            if (bind(bus_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                std::cerr << "bind bus_socket broke" << std::endl;
                //blow up exit(1)?
            }

            int enable_canfd = 1;
            if (setsockopt(bus_socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)) != 0) {
                std::cerr << "socket did not set to CAN_RAW_FD_FRAMES" << std::endl;
                //blow up exit(1)?
            }
        }

    public:
        Logger(std::string bus_name, Auth &server_info, std::unordered_set<int> can_ids_listen, bool log_all, std::string file_path/*, std::istream &is*/) : can_bus_name(bus_name), 
        si(server_info.host, server_info.port, server_info.db_name, server_info.user, server_info.password), listen_ids(can_ids_listen),
        log_all(log_all), file_path(file_path) {
            /*int id;
            while (is >> id) {
                valid_ids.emplace(id);
            }
            */
        }

        void start() {
            init_bus();
            struct canfd_frame cfd;
            while (true) {                                                              //catch an interupt instead?

                //read a vcan message from bus (can_id)
                //insert into buffer
                //call to decode
                //push to influx and logger (asci)
                //done

                ssize_t bytes_read;
                while (bytes_read = read(bus_socket, &cfd, CANFD_MTU)) {                //CANFD_MTU is macro for sizeof(canfd_frame)
                    if (bytes_read != CANFD_MTU) {
                        std::cerr << "read a can frame of different size than canfd, expected: " 
                        << CANFD_MTU << " got: " << bytes_read << std::endl;
                        exit(1);
                    }
                    /*if (valid_ids.find(cfd.can_id) == valid_ids.end()) {
                        std::cerr << "read a canid that is not part of the valid set" << std::endl;
                        //probably should log here
                    }*/
                    buffer.emplace(cfd);
                }
                cfd = buffer.front();
                std::string resp;

                
                std::string decoded_message = decode(cfd.can_id);                      //calling decode
                long long ts = now_ns();
                
                int ret = influxdb_cpp::builder()
                    .meas("message")
                    .tag("message", "can id")
                    .field("message", decoded_message)
                    .field("can id", static_cast<int>(cfd.can_id))
                    .timestamp(ts)
                    .post_http(si, &resp);
            }
        }
        
        void print(std::ostream &os) {
            os << "Auth:\n"
            << "\t - Host: " << si.host_ << "\n"
            << "\t - Port: " << si.port_ << "\n"
            << "\t - Db_name: " << si.db_ << "\n"
            << "\t - User: " << si.usr_ << "\n"
            << "\t - Password: " << si.pwd_ << "\n";

            os << "\n"
            << "Info:\n"
            << "\t - Name: " << can_bus_name << "\n"
            << "\t - log_all: " << log_all << "\n"
            << "\t - file_path: " << file_path << "\n"
            << "\t - log_specify: ";
            auto it = listen_ids.begin();
            while (it != listen_ids.end()) {
                os << *it << ", ";
                it++;
            }
            os << std::endl;
        }

};

void logger_factory(std::vector<Logger> &loggers, std::string path, bool debug = false) {
    //will init a vector of configured Loggers from a yaml found at path

    int size = 0;
    Yaml::Node root;
    try {
        Yaml::Parse(root, path.c_str());
    } catch (const Yaml::Exception e) {
        std::cerr << "cautch YAML error while parsing: " << e.what() << std::endl;
    }
    size = root["logger_bus_size"].As<int>();
    if (size > 4) {
        std::cerr << "cannot support more than 4 can busses, recieved bus size of: " << size << std::endl;
    }
    if (debug) std::cout << "Size: " << size << std::endl;

    Yaml::Node auth_node = root["auth"][0];
    std::string host = auth_node["host"].As<std::string>();
    int port = auth_node["port"].As<int>();
    std::string db_name = auth_node["db_name"].As<std::string>();
    std::string user = auth_node["user"].As<std::string>();
    std::string password = auth_node["password"].As<std::string>();
    Auth auth =  {host, port, db_name, user, password};

    if (debug) std::cout << "host: " << host << ", port: " << port << ", db_name: " << db_name << ", user " << user << ", password: " << password << std::endl;

    Yaml::Node loggers_node = root["loggers"];
    loggers.reserve(size);   
    for (int i = 0; i < size; ++i) {
        std::string name = loggers_node[i]["name"].As<std::string>();
        bool log_all = loggers_node[i]["log_all"].As<bool>();
        std::string log_spec_str = loggers_node[i]["log_specify"].As<std::string>();
        std::unordered_set<int> log_ids;
        std::string num = "";
        for (size_t j = 0; j < log_spec_str.size(); ++j) {
            if (log_spec_str[j] == ',') {
                log_ids.insert(std::stoi(num));
                num.clear();
            } else {
                num.push_back(log_spec_str[j]);
            }
        }
        if (num.size()) {
            log_ids.insert(std::stoi(num));
        }
        std::string file_path = loggers_node[i]["file_path"].As<std::string>();
        loggers.emplace_back(name, auth, log_ids, log_all, file_path);
        if (debug) std::cout << "name: " << name << ", log_all: " << log_all << ", file_path: " << file_path << std::endl;
    }
}
