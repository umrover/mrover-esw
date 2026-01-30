#include "logger.hpp"

namespace logger {

//GLOBALS
std::atomic<bool> running = true;
std::mutex cout_mutex;

Auth::Auth(
    std::string host, int port, 
    std::string database, 
    std::string user, 
    std::string password) 
      : host(std::move(host)), 
        port(port), 
        db_name(std::move(database)), 
        user(std::move(user)), 
        password(std::move(password)) {}

void Logger::DynamicBuilder::post(
    const std::string &measurement,
    const std::string &bus_name,
    const std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue> &data,
    const long long timestamp) {
        
        _m(measurement);

        _t("bus_name", bus_name);

        bool is_first_field = true;

        for (const auto &[name, value] : data) {
            char delim = is_first_field ? ' ' : ',';

            if (value.is_floating_point()) {
                _f_f(delim, name, value.as_double(), 8);
            } else if (value.is_integral()) {
                _f_i(delim, name, value.as_signed_integer());
            } else if (value.is_string()) {
                _f_s(delim, name, value.as_string());
            } else {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "something weird happened";
            }
        }
    }


std::string make_can_timestamp() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto us  = duration_cast<microseconds>(now.time_since_epoch()).count();

    long long sec = us / 1'000'000;
    long long micros = us % 1'000'000;
    char buf[32];  // plenty for "(1234567890.123456)"

    std::snprintf(buf, sizeof(buf), "(%lld.%06lld)", sec, micros);
    return std::string(buf);
}

long long now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
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

        for (const auto &can_frame : local_buffer) {
            const auto *desc = parser.message(can_frame.id);
            builder.post(desc->name(), can_bus_name, can_frame.data, can_frame.time);
        }
    }

    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        
        for (const auto &can_frame : buffer) {
            const auto *desc = parser.message(can_frame.id);
            builder.post(desc->name(), can_bus_name, can_frame.data, can_frame.time);
        }
    }
}

void Logger::_init_bus() {
    
    //parse files

    for (auto it = begin(dbc_file_paths); it != end(dbc_file_paths); ++it) {
        parser.parse(*it);
    }

    //add log messasges into the processor

    for (auto it = begin(log_ids); it != end(log_ids); ++it) {
        mrover::dbc_runtime::CanMessageDescription const *message = parser.message(*it);
        processor.add_message_description(*message);
    }

    bus_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (bus_socket < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(std::strerror(errno)));
    }
    struct ifreq ifr;
    struct sockaddr_can addr;

    strcpy(ifr.ifr_name, can_bus_name.c_str());

    if (ioctl(bus_socket, SIOCGIFINDEX, &ifr) < 0) {
        throw std::runtime_error("ioctl broke: " + can_bus_name);
        //blow up
    }

    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(bus_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("bind broke: " + can_bus_name);   
    }

    int enable_canfd = 1;
    if (setsockopt(bus_socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)) != 0) {
        throw std::runtime_error("socket broke: " + can_bus_name);
        //blow up
    }
}
        
void Logger::_log_ascii(unsigned char *arr, std::string name, std::ofstream &outputFile, uint32_t id) {
    if (!outputFile.is_open()) return;
    

    outputFile << make_can_timestamp() << can_bus_name << " ";

    //TODO fix this! should be the proper CAN id

    outputFile << id << "#";

    
    for (size_t i = 0; i < CANFD_MAX_DLEN; i++) {
    outputFile << std::hex              // switch to hex mode
               << std::uppercase        // use uppercase letters (A–F)
               << std::setw(2)          // pad to 2 digits
               << std::setfill('0')     // pad with '0' if needed
               << static_cast<int>(arr[i] & 0xFF);  // ensure unsigned
    }   

    outputFile << std::dec << "\n";
}

auto Logger::_decode(const uint32_t id, const canfd_frame &can_frame) -> std::unordered_map<std::string, mrover::dbc_runtime::CanSignalValue> {
    return processor.decode(id, std::string_view(reinterpret_cast<const char*>(can_frame.data), can_frame.len));
}

Logger::Logger(std::string &bus_name, 
               std::string &yaml_file_path, 
               std::string &ascii_log_file_path, 
               Auth &server_info,
               std::unordered_set<int> &&log_ids, 
               std::unordered_set<std::string> &&dbc_file_paths,
               bool log_all, 
               bool debug)

  : can_bus_name(bus_name), 
    yaml_file_path(yaml_file_path), 
    ascii_log_file_path(ascii_log_file_path), 
    si(server_info.db_name, server_info.port, server_info.host, server_info.user, server_info.password), 
    log_ids(log_ids), 
    dbc_file_paths(std::move(dbc_file_paths)), 
    log_all(log_all), 
    debug(debug)
{}

Logger::Logger(logger::Logger &&other) noexcept
  : can_bus_name(std::move(other.can_bus_name)),
    yaml_file_path(std::move(other.yaml_file_path)),
    ascii_log_file_path(std::move(other.ascii_log_file_path)),
    si(other.si),
    log_ids(std::move(other.log_ids)),
    dbc_file_paths(std::move(other.dbc_file_paths)),
    log_all(other.log_all),
    debug(other.debug)
{}



void Logger::start() {
    _init_bus();

    std::filesystem::path dir = std::filesystem::path(ascii_log_file_path).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Created directory: " << dir << std::endl;
        }
    }

    // Open log file (ofstream will create it if it doesn't exist)
    std::ofstream file(ascii_log_file_path, std::ios::app); // use app to append
    if (!file.is_open()) {
        throw std::runtime_error("Could not open log file: " + ascii_log_file_path);
    }

    if (debug) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Logger started for " << can_bus_name << std::endl;
    }
    
    committer_thread = std::thread(&Logger::_committer_worker, this);

    struct canfd_frame cfd;


    while (running) {                                                              //catch an interupt instead?

        //read a vcan message from bus (can_id)
        //insert into buffer
        //call to decode
        //push to influx and logger (asci)
        //done

        ssize_t bytes_read = read(bus_socket, &cfd, CANFD_MTU);               //CANFD_MTU is macro for sizeof(canfd_frame)
        
        if (bytes_read < 0) {
            ++read_error_count;
            {   
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "Read error on " << can_bus_name << ": " << std::strerror(errno) << std::endl;
            }
            continue;
        }
        if (bytes_read < (ssize_t)CAN_MTU) {
            ++read_error_count_incomplete;
            continue;
        }

        uint32_t id = cfd.can_id & CAN_EFF_MASK;

        logger::Logger::_log_ascii(cfd.data, can_bus_name, file, id);

        std::string resp;

        DecodedFrame decoded_message = {.id = id, .time=now_ns(), .data=_decode(id, cfd)};

        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            buffer.push_back(decoded_message);
        }
        cv.notify_one();

    } //endwhile

    if (committer_thread.joinable()) committer_thread.join();
}

void Logger::print(std::ostream &os) {
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
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
        << "\t - yaml_file_path: " << yaml_file_path << "\n"
        << "\t - log_specify: ";
        auto it = log_ids.begin();
        while (it != log_ids.end()) {
            os << *it << ", ";
            it++;
        }
        os << std::endl;
    }
}


std::vector<Logger> logger_factory(std::string &yaml_path, bool debug) {
    //will init a vector of configured Loggers from a yaml found at path
    std::vector<Logger> loggers;

    int size = 0;
    if (debug) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "parsing" << std::endl;
    }
    Yaml::Node root;
    try {
        Yaml::Parse(root, yaml_path.c_str());
    } catch (const Yaml::Exception &e) {
        throw std::runtime_error("yaml parse broke");
    }
    if (debug) std::cout << "parsing 1" << std::endl;
    size = root["logger_bus_size"].As<int>();
    if (size > 4) {
        std::lock_guard<std::mutex> lock(cout_mutex);
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

    if (debug) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "host: " << host << ", port: " << port << ", db_name: " << db_name << ", user " << user << ", password: " << password << std::endl;
    }

    Yaml::Node loggers_node = root["loggers"];
    loggers.reserve(size);   

    for (int i = 0; i < size; ++i) {
        if (debug) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "in logger factory, iteration:" << i << "\n";
        }
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

        std::string dbc_file_paths_str = loggers_node[i]["dbc_file_paths"].As<std::string>();
        std::unordered_set<std::string> dbc_file_paths;
        std::string dbc_file_path = "";
        size_t j = 0;
        while (j < dbc_file_paths_str.size() && dbc_file_paths_str[j] == ' ') ++j;          //leading whitespace
        for (;j < dbc_file_paths_str.size();) {
            if (dbc_file_paths_str[j] == ',') {
                auto it = dbc_file_paths.find(dbc_file_path);
                if (it != dbc_file_paths.end()) {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cerr << "found duplicate file path in yaml in logger: " << i << ", duplicate path: " << dbc_file_path << "\n";
                } else {
                    dbc_file_paths.insert(dbc_file_path);
                }
                dbc_file_path.clear();

                ++j;
                while (j < dbc_file_paths_str.size() && dbc_file_paths_str[j] == ' ') ++j;
            } else {
                dbc_file_path.push_back(dbc_file_paths_str[j]);
                ++j;
            }
        }
        if (dbc_file_path.size()) {
            auto it = dbc_file_paths.find(dbc_file_path);
            if (it != dbc_file_paths.end()) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "found duplicate file path in yaml in logger: " << i << ", duplicate path: " << dbc_file_path << "\n";
            } else {
                dbc_file_paths.insert(dbc_file_path);
            }
        }

        if (dbc_file_paths.empty()) {
            std::string error = "CAN Bus: " + std::to_string(i) + " parsed 0 file paths";
            throw std::runtime_error(error);
        }

        std::string ascii_file_path = loggers_node[i]["ascii_file_path"].As<std::string>();

        loggers.emplace_back(name, yaml_path, ascii_file_path, auth, std::move(log_ids), std::move(dbc_file_paths), log_all, debug);
        if (debug) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "name: " << name << ", log_all: " << log_all << ", file_path: " << dbc_file_path << std::endl;
        }
    } //endfor

    return loggers;
}

void handle_SIGINT(int) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "caught sigint\n";
    running.store(false);
}

void run_bus(std::vector<Logger> &loggers) {
    std::signal(SIGINT, handle_SIGINT);

    std::vector<std::thread> threads;

    for (int i = 0; i < static_cast<int>(loggers.size()); ++i) {
        threads.emplace_back(&Logger::start, &loggers[i]);
    }

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto &thread : threads) {
        if (thread.joinable()) thread.join();
    }
}

} // Logger namespace