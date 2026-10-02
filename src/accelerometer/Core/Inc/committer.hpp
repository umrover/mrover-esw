#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "influxdb.hpp"
#include "mpu6050.hpp"

namespace accel {

    /**
     * @brief Batches IMU samples from every bus and periodically POSTs them to InfluxDB
     *
     * Pollers call push() from their own threads; run() sends everything queued once per
     * commit period, so InfluxDB sees one HTTP request per period instead of one per sample.
     * A failed POST drops that batch.
     */
    class Committer {
    private:
        struct Entry {
            std::string bus_name;
            ImuSample sample;
        };

        struct DynamicBuilder : public influxdb_cpp::builder {
            /**
             * @brief Append one line: <measurement>,bus_name=<bus> x=..,y=..,z=.. <timestamp>
             *
             * @param measurement Name of the measurement ("accelerometer" or "gyroscope")
             * @param bus_name Name of bus sourcing data
             * @param value Three axis reading
             * @param timestamp Time in ms the sample was taken
             */
            void post(std::string const& measurement, std::string const& bus_name, Vec3 const& value, long long timestamp);

            /**
             * @brief Commit POST request buffer to InfluxDB
             *
             * @param si Server info of the InfluxDB instance
             *
             * @return return code of _post_http(), 0 on success
             */
            auto commit(influxdb_cpp::server_info const& si) -> int;
        };

        influxdb_cpp::server_info si;
        std::chrono::milliseconds period;
        std::atomic<bool> running{true};

        std::mutex buffer_mutex;
        std::vector<Entry> buffer;

        DynamicBuilder builder;
        uint64_t influx_post_error_count = 0;

        void _flush();

    public:
        Committer(influxdb_cpp::server_info si, std::chrono::milliseconds period);

        /**
         * @brief Queue a sample, thread safe
         *
         * @param bus_name I2C bus the sample came from, written as the bus_name tag
         * @param sample Sample to write
         */
        void push(std::string const& bus_name, ImuSample const& sample);

        /**
         * @brief Commit loop, does a final flush and returns after stop()
         */
        void run();
        void stop();

        [[nodiscard]] auto error_count() const -> uint64_t { return influx_post_error_count; }
    };

} // namespace accel
