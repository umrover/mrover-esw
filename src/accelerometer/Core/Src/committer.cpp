#include "committer.hpp"

#include <format>
#include <iostream>
#include <thread>

namespace accel {

    namespace {
        constexpr int FIELD_PRECISION = 5;
        constexpr unsigned POST_TIMEOUT_S = 2; // without a timeout a hung InfluxDB blocks forever
    } // namespace

    void Committer::DynamicBuilder::post(
            std::string const& measurement,
            std::string const& bus_name,
            Vec3 const& value,
            long long const timestamp) {

        if (lines_.tellp() > 0) {
            lines_ << '\n';
        }

        _m(measurement);
        _t("bus_name", bus_name);
        _f_f(' ', "x", value.x, FIELD_PRECISION);
        _f_f(',', "y", value.y, FIELD_PRECISION);
        _f_f(',', "z", value.z, FIELD_PRECISION);
        _ts(timestamp);
    }

    auto Committer::DynamicBuilder::commit(influxdb_cpp::server_info const& si) -> int {
        if (lines_.tellp() == 0) return 0;

        std::string resp;
        int ret = _post_http(si, &resp, POST_TIMEOUT_S);

        lines_.str("");
        lines_.clear();

        return ret;
    }

    Committer::Committer(influxdb_cpp::server_info si, std::chrono::milliseconds period)
        : si(std::move(si)),
          period(period) {}

    void Committer::push(std::string const& bus_name, ImuSample const& sample) {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        buffer.push_back({bus_name, sample});
    }

    void Committer::_flush() {
        std::vector<Entry> entries;
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            entries.swap(buffer);
        }

        for (auto const& [bus_name, sample]: entries) {
            builder.post("accelerometer", bus_name, sample.accel, sample.time);
            builder.post("gyroscope", bus_name, sample.gyro, sample.time);
        }

        int const status = builder.commit(si);
        if (status != 0) {
            ++influx_post_error_count;
            std::cerr << std::format("influx commit failed with error: {}, dropped {} samples\n", status, entries.size());
        }
    }

    void Committer::run() {
        auto next = std::chrono::steady_clock::now() + period;

        while (running.load()) {
            std::this_thread::sleep_until(next);
            _flush();

            // if a slow POST put us behind, skip missed periods instead of bursting
            auto const now = std::chrono::steady_clock::now();
            next += period;
            if (next <= now) next = now + period;
        }

        _flush();
    }

    void Committer::stop() {
        running.store(false);
    }

} // namespace accel
