#include "logger.hpp"
#include "Yaml.hpp"
#include "file_parser.hpp"
#include "influxdb.hpp"


#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace logger {

    void Logger::DynamicBuilder::post(
            std::string const& measurement,
            std::string const& bus_name,
            std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue> const& data,
            long long const timestamp) {

        if (lines_.tellp() > 0) {
            lines_ << '\n';
        }
        _m(measurement);

        _t("bus_name", bus_name);

        bool is_first_field = true;

        for (auto const& [name, value]: data) {
            char delim = is_first_field ? ' ' : ',';
            is_first_field = false;

            if (value.is_floating_point()) {
                _f_f(delim, name, value.as_double(), 8);
            } else if (value.is_integral()) {
                _f_i(delim, name, value.as_signed_integer());
            } else if (value.is_string()) {
                _f_s(delim, name, value.as_string());
            } else {
                throw std::runtime_error(std::format("can value with name: {}, is not floating, integral, or string type", name));
            }
        }

        _ts(timestamp);
    }

    auto Logger::DynamicBuilder::commit(influxdb_cpp::server_info const& si) -> int {
        if (lines_.tellp() == 0) return 0;

        std::string resp;
        int ret = _post_http(si, &resp);

        lines_.str("");
        lines_.clear();

        return ret;
    }

    auto make_can_timestamp() -> std::string {
        using namespace std::chrono;

        auto now = system_clock::now();
        auto us = duration_cast<microseconds>(now.time_since_epoch()).count();

        long long sec = us / 1'000'000;
        long long micros = us % 1'000'000;
        char buf[32];

        std::snprintf(buf, sizeof(buf), "(%lld.%06lld)", sec, micros);
        return {buf};
    }

    auto now_ms() -> long long {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    void Logger::_committer_worker() {

        std::vector<DecodedFrame> local_buffer;

        while (true) {
            {
                std::unique_lock<std::mutex> lock(buffer_mutex);
                cv.wait(lock, [this] { return !buffer.empty() || !running.load(); });

                if (!running.load() && buffer.empty()) break;

                while (!buffer.empty()) {
                    local_buffer.push_back(std::move(buffer.front()));
                    buffer.pop_front();
                }
            }

            for (auto const& can_frame: local_buffer) {
                auto const desc = parser.message(can_frame.id);
                if (desc == nullptr) throw std::runtime_error(std::format("failed to get description for {:x}", can_frame.id));

                builder.post(desc->name(), can_bus_name, can_frame.data, can_frame.time);
                int status = builder.commit(si);
                if (status != 0) {
                    {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "build commit failed with error " << status << "\n";
                        ++influx_post_error_count;
                    }
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(buffer_mutex);

            for (auto const& can_frame: buffer) {
                auto desc = parser.message(can_frame.id);
                builder.post(desc->name(), can_bus_name, can_frame.data, can_frame.time);
                builder.commit(si);
            }
        }
    }

    void Logger::_init_bus() {

        //parse files

        for (auto const& dbc_file_path: dbc_file_paths) {
            if (!parser.parse(dbc_file_path)) {
                throw std::runtime_error(std::format("failed to parse file: {} with error: {}, lines parsed: {}", dbc_file_path, mrover::dbc_runtime::CanDbcFileParser::to_string(parser.error()), parser.lines_parsed()));
            }
        }

        //add log messasges into the processor
        if (mode == log_mode::WHITELIST_IDS) {
            for (unsigned int log_id: log_ids) {
                auto const message = parser.message(log_id);
                if (!message) throw std::runtime_error(std::format("parser failed to fetch message with id: {}, error: {}", std::to_string(log_id), mrover::dbc_runtime::CanDbcFileParser::to_string(parser.error())));
                processor.add_message_description(*message);
            }
        } else if (mode == log_mode::BLACKLIST_IDS) {
            for (auto const& message: parser.messages()) {
                processor.add_message_description(message);
            }
        }

        //create socket
        bus_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (bus_socket < 0) throw std::runtime_error("Failed to create socket: " + std::string(std::strerror(errno)));

        int flags = fcntl(bus_socket, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("socket flags failed");
        }
        if (fcntl(bus_socket, F_SETFL, flags | O_NONBLOCK) == -1) throw std::runtime_error("Failed to make socket nonblock");

        struct ifreq ifr{};
        struct sockaddr_can addr{};

        std::strncpy(ifr.ifr_name, can_bus_name.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        if (ioctl(bus_socket, SIOCGIFINDEX, &ifr) < 0) {
            throw std::runtime_error("ioctl broke: " + can_bus_name);
            //blow up
        }

        std::memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(bus_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
            throw std::runtime_error("bind broke: " + can_bus_name);
        }

        int enable_canfd = 1;
        if (setsockopt(bus_socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)) != 0) {
            throw std::runtime_error("socket broke: " + can_bus_name);
            //blow up
        }
    }

    void Logger::_log_ascii(unsigned char const* arr, std::string const& name, std::ofstream& outputFile, uint32_t id) {
        if (!outputFile.is_open()) return;

        outputFile << make_can_timestamp() << can_bus_name << " ";

        outputFile << id << "#";

        // print two digit unsigned
        for (size_t i = 0; i < CANFD_MAX_DLEN; i++) {
            outputFile << std::hex
                       << std::uppercase
                       << std::setw(2)
                       << std::setfill('0')
                       << static_cast<int>(arr[i] & 0xFF);
        }

        outputFile << std::dec << "\n";
    }

    auto Logger::_decode(uint32_t const id, canfd_frame const& can_frame) -> std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue> {
        return processor.decode(id, std::string_view(reinterpret_cast<char const*>(can_frame.data), can_frame.len));
    }

    Logger::Logger(int id,
                   std::string& bus_name,
                   std::string& yaml_file_path,
                   std::unordered_set<uint32_t>&& log_ids,
                   std::unordered_set<std::string>&& dbc_file_paths,
                   influxdb_cpp::server_info& si,
                   log_mode mode,
                   bool log_ascii)

        : id(id),
          can_bus_name(bus_name),
          yaml_file_path(yaml_file_path),
          log_ascii(log_ascii),
          si(si),
          log_ids(log_ids),
          dbc_file_paths(std::move(dbc_file_paths)),
          mode(mode) {}

    Logger::Logger(logger::Logger&& other) noexcept
        : id(other.id),
          can_bus_name(std::move(other.can_bus_name)),
          yaml_file_path(std::move(other.yaml_file_path)),
          log_ascii(other.log_ascii),
          si(std::move(other.si)),
          log_ids(std::move(other.log_ids)),
          dbc_file_paths(std::move(other.dbc_file_paths)),
          mode(other.mode),
          debug(other.debug) {}


    void Logger::start() {
        try {
            _init_bus();
            std::flush(std::cout);
            // std::filesystem::path dir = std::filesystem::path(ascii_log_file_path).parent_path();
            // if (!dir.empty() && !std::filesystem::exists(dir)) {
            //     std::filesystem::create_directories(dir);
            //     {
            //         std::lock_guard<std::mutex> lock(cout_mutex);
            //         std::cout << "Created directory: " << dir << "\n";
            //     }
            // }

        } catch (std::exception const& e) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cerr << "Logger thread failed: " << e.what() << "\n";
        } catch (...) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cerr << "Logger thread failed with unknown exception\n";
        }

        // TODO: create a default ascii_log_file_path with log_ascii + can_bus_name, maybe optional arg
        // Open log file (ofstream will create it if it doesn't exist)
        // std::ofstream file(ascii_log_file_path, std::ios::app); // use app to append
        // if (!file.is_open()) {
        //     std::lock_guard<std::mutex> lock(cout_mutex);
        //     std::cerr << "Logger cannot open file" << ascii_log_file_path << "\n";
        //     return;
        // }

        if (debug) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Logger started for " << can_bus_name << "\n";
        }

        committer_thread = std::thread(&Logger::_committer_worker, this);

        struct canfd_frame cfd{};


        while (running.load()) { //catch an interupt instead?

            //read a vcan message from bus (can_id)
            //insert into buffer
            //call to decode
            //push to influx and logger (asci)
            //done

            ssize_t bytes_read = read(bus_socket, &cfd, CANFD_MTU); //CANFD_MTU is macro for sizeof(canfd_frame)

            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                    ;
                } else {
                    ++read_error_count;
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cerr << "Read error on " << can_bus_name << ": " << std::strerror(errno) << "\n";
                    continue;
                }
            }
            if (bytes_read < (ssize_t) CAN_MTU) {
                ++read_error_count_incomplete;
                continue;
            }

            uint32_t id = (cfd.can_id & CAN_EFF_MASK & 0xFFFF0000) | 0x80000000; //hacky fix

            switch (mode) {
                case log_mode::WHITELIST_IDS: {
                    if (log_ids.find(id) == log_ids.end()) continue;
                    break;
                }
                case log_mode::BLACKLIST_IDS: {
                    if (log_ids.find(id) != log_ids.end()) continue;
                    break;
                }
            }
            // TODO: Fix, need a file path
            //logger::Logger::_log_ascii(cfd.data, can_bus_name, file, id);

            DecodedFrame decoded_message = {.id = id, .time = now_ms(), .data = _decode(id, cfd)};

            {
                std::lock_guard<std::mutex> lock(buffer_mutex);
                buffer.push_back(decoded_message);
            }
            cv.notify_one();

        } //endwhile

        cv.notify_one();

        if (committer_thread.joinable()) committer_thread.join();
    }

    void Logger::print() const {
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Auth:\n"
                      << "\t - Host: " << si.host_ << "\n"
                      << "\t - Port: " << si.port_ << "\n"
                      << "\t - Db_name: " << si.db_ << "\n"
                      << "\t - User: " << si.usr_ << "\n"
                      << "\t - Password: " << si.pwd_ << "\n";

            std::cout << "\n"
                      << "Info:\n"
                      << "\t - Name: " << can_bus_name << "\n"
                      << "\t - Id: " << id << "\n"
                      << "\t - log_mode: " << static_cast<int>(mode) << "\n"
                      << "\t - yaml_file_path: " << yaml_file_path << "\n"
                      << "\t - log_specify: ";
            auto it = log_ids.begin();
            while (it != log_ids.end()) {
                std::cout << *it << ", ";
                it++;
            }
            std::cout << "\n";
        }
    }

    void Logger::print_error() const {
        if (debug) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Logger: " << id << " | read_error_count: " << read_error_count << " | incomplete reads: " << read_error_count_incomplete << " | influx post errors: " << influx_post_error_count << "\n";
        }
    }

    // End logger class members

    auto logger_factory(std::string& yaml_path) -> std::vector<Logger> {
        std::vector<Logger> loggers;

        char const* dbc_root_env = std::getenv("DBC_ROOT");
        if (!dbc_root_env) throw std::runtime_error("DBC_ROOT environment variable is not set");

        std::string dbc_root_path = dbc_root_env;
        int size = 0;

        Yaml::Node root;
        try {
            Yaml::Parse(root, yaml_path.c_str());
        } catch (Yaml::Exception const& e) {
            throw std::runtime_error(std::format("yaml parsing broke with error: {}\n", e.what()));
        }

        size = root["logger_bus_size"].As<int>();
        if (size > 4) throw std::runtime_error(std::format("recieved logger bus size of {}, which is larger than 4", size));

        std::cout << "Size: " << size << std::endl;

        char const* env_host = std::getenv("INFLUXDB_HOST");
        if (!env_host) throw std::runtime_error("influxdb environment variable unset: host");
        std::string host(env_host);

        char const* env_port = std::getenv("INFLUXDB_PORT");
        if (!env_port) throw std::runtime_error("influxdb environment variable unset: port");
        int port = std::stoi(env_port);

        char const* env_db = std::getenv("INFLUXDB_DB");
        if (!env_db) throw std::runtime_error("influxdb environment variable unset: db");
        std::string db_name(env_db);

        char const* env_user = std::getenv("INFLUXDB_USER");
        if (!env_user) throw std::runtime_error("influxdb environment variable unset: user");
        std::string user(env_user);

        char const* env_pass = std::getenv("INFLUXDB_PASSWORD");
        if (!env_port) throw std::runtime_error("influxdb environment variable unset: password");
        std::string password(env_pass);

        influxdb_cpp::server_info auth(host, port, db_name, user, password);

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "host: " << host << ", port: " << port << ", db_name: " << db_name << ", user " << user << ", password: " << password << std::endl;
        }

        Yaml::Node loggers_node = root["loggers"];
        loggers.reserve(size);

        for (int i = 0; i < size; ++i) {

            auto name = loggers_node[i]["name"].As<std::string>();
            auto log_mode_str = loggers_node[i]["log_mode"].As<std::string>();

            log_mode mode;
            if (log_mode_str == "whitelist") {
                mode = log_mode::WHITELIST_IDS;
            } else if (log_mode_str == "blacklist") {
                mode = log_mode::BLACKLIST_IDS;
            } else {
                throw std::runtime_error(std::format("expected 'whitelist' or 'blacklist' but recieved {}", log_mode_str));
            }

            auto log_spec_str = loggers_node[i]["log_specify"].As<std::string>();
            std::unordered_set<uint32_t> log_ids;

            std::stringstream log_specify_stream(log_spec_str);
            std::string token;
            while (std::getline(log_specify_stream, token, ',')) {
                try {
                    auto clean = trim(token);
                    if (clean.empty()) continue;
                    log_ids.insert(std::stoi(clean, nullptr, 0));
                } catch (std::invalid_argument const& e) {
                    throw std::runtime_error(std::format("non-intger arg to stoi in log_specify: {}", e.what()));
                } catch (...) {
                    throw std::runtime_error(std::format("log_specify stoi broke parsing token: {}", token));
                }
            }

            auto dbc_file_paths_str = loggers_node[i]["dbc_paths"].As<std::string>();
            std::unordered_set<std::string> dbc_file_paths;

            std::stringstream dbc_stream(dbc_file_paths_str);

            while (std::getline(dbc_stream, token, ',')) {
                token = trim(token);
                std::string full_path = dbc_root_path + token;
                if (!std::filesystem::exists(full_path)) {
                    throw std::runtime_error(std::format("couldn't find path: {} in filesystem", full_path));
                }

                if (full_path.empty()) continue;
                //insert returns a pair <it, bool>
                if (!dbc_file_paths.insert(full_path).second) {
                    throw std::runtime_error(std::format("found duplicate file path in {} with value {}", i, full_path));
                }
            }

            if (dbc_file_paths.empty()) {
                throw std::runtime_error(std::format("found no dbc file paths"));
            }

            bool log_ascii = loggers_node[i]["log_ascii"].As<bool>();

            loggers.emplace_back(i, name, yaml_path, std::move(log_ids), std::move(dbc_file_paths), auth, mode, log_ascii);
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "name: " << name << ", log_mode: " << static_cast<int>(mode) << ", file_path: " << dbc_root_path << std::endl;
            }
        } //endfor

        return loggers;
    }

    void handle_SIGINT(int) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\ncaught sigint\n";
        running.store(false);
    }

    void run_bus(std::vector<Logger>& loggers) {
        std::signal(SIGINT, handle_SIGINT);

        std::vector<std::thread> threads;

        threads.reserve(static_cast<int>(loggers.size()));
        for (auto& logger: loggers) {
            threads.emplace_back(&Logger::start, &logger);
        }

        for (auto& thread: threads) {
            if (thread.joinable()) thread.join();
        }

        for (auto& logger: loggers) {
            logger.print_error();
        }
    }

    static auto trim(std::string const& s) -> std::string {
        auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char c) {
            return std::isspace(c);
        });

        auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) {
                       return std::isspace(c);
                   }).base();

        if (start >= end) return "";
        return {start, end};
    }

} // namespace logger
