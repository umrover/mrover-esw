#include "Yaml.hpp"
#include "committer.hpp"
#include "i2c_bus.hpp"
#include "mpu6050.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

    std::atomic<bool> running{true};

    void handle_signal(int) { running.store(false); }

    auto require_env(char const* name) -> std::string {
        char const* value = std::getenv(name);
        if (!value) throw std::runtime_error(std::format("influxdb environment variable unset: {}", name));
        return value;
    }

    /**
     * @brief Poll the MPU-6050 on one bus until shutdown
     *
     * The sensor is re-initialized after a failed read, so it can be
     * unplugged and reconnected without restarting the container.
     *
     * @param bus Opened bus with one MPU-6050 at AD0 low
     * @param period Time between polls
     * @param committer Destination for samples
     */
    void poll_bus(accel::I2CBus& bus, std::chrono::microseconds period, accel::Committer& committer) {
        accel::MPU6050 imu(bus);
        bool ready = false;
        bool reported_missing = false;
        auto next = std::chrono::steady_clock::now();

        while (running.load()) {
            if (!ready) {
                ready = imu.init();
                if (!ready) {
                    if (!reported_missing) std::cerr << std::format("{}: MPU-6050 not responding, retrying\n", bus.name());
                    reported_missing = true;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    next = std::chrono::steady_clock::now();
                    continue;
                }
                reported_missing = false;
                std::cout << std::format("{}: MPU-6050 initialized\n", bus.name());
            }

            accel::ImuSample sample{};
            if (!imu.read(sample)) {
                std::cerr << std::format("{}: read failed, reinitializing\n", bus.name());
                ready = false;
                continue;
            }
            committer.push(bus.name(), sample);

            next += period;
            auto const now = std::chrono::steady_clock::now();
            if (next <= now) next = now + period;
            std::this_thread::sleep_until(next);
        }
    }

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage: ./accelerometer <path_to_yaml>\n";
        return 1;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal); // docker stop

    std::vector<std::unique_ptr<accel::I2CBus>> buses;
    std::chrono::microseconds poll_period{};
    std::chrono::milliseconds commit_period{};
    std::unique_ptr<influxdb_cpp::server_info> si;

    try {
        Yaml::Node root;
        Yaml::Parse(root, argv[1]);

        // As<int>() is uninitialized on a missing key, default to 0 so it fails the check below
        auto const poll_rate_hz = root["poll_rate_hz"].As<int>(0);
        auto const commit_period_ms = root["commit_period_ms"].As<int>(0);
        if (poll_rate_hz <= 0 || commit_period_ms <= 0) throw std::runtime_error("poll_rate_hz and commit_period_ms must be positive");
        poll_period = std::chrono::microseconds(1'000'000 / poll_rate_hz);
        commit_period = std::chrono::milliseconds(commit_period_ms);

        // a missing /dev/i2c-N is a config error, fail before starting anything
        Yaml::Node& buses_node = root["buses"];
        for (size_t i = 0; i < buses_node.Size(); ++i) {
            auto& bus = buses.emplace_back(std::make_unique<accel::I2CBus>(buses_node[i].As<std::string>()));
            bus->open();
        }
        if (buses.empty()) throw std::runtime_error("no buses listed in config");

        si = std::make_unique<influxdb_cpp::server_info>(
                require_env("INFLUXDB_HOST"),
                std::stoi(require_env("INFLUXDB_PORT")),
                require_env("INFLUXDB_DB"),
                require_env("INFLUXDB_USER"),
                require_env("INFLUXDB_PASSWORD"));

        std::cout << std::format("Using config: {}\n", root["name"].As<std::string>());
        std::cout << std::format("Polling {} bus(es) at {} Hz, committing every {} ms to {}:{}/{}\n",
                                 buses.size(), poll_rate_hz, commit_period_ms, si->host_, si->port_, si->db_);
    } catch (Yaml::Exception const& e) {
        std::cerr << std::format("yaml parsing broke with error: {}\n", e.what());
        return 1;
    } catch (std::exception const& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    accel::Committer committer(*si, commit_period);
    std::thread committer_thread(&accel::Committer::run, &committer);

    std::vector<std::thread> pollers;
    pollers.reserve(buses.size());
    for (auto& bus: buses) {
        pollers.emplace_back(poll_bus, std::ref(*bus), poll_period, std::ref(committer));
    }

    // stop pollers first so their last samples make it into the final flush
    for (auto& poller: pollers) poller.join();
    committer.stop();
    committer_thread.join();

    std::cout << std::format("shutdown, influx post errors: {}\n", committer.error_count());
    return 0;
}
