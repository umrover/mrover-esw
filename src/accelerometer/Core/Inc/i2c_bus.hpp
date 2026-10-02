#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace accel {

    /**
     * @brief Thin wrapper around a Linux /dev/i2c-N character device (linux/i2c-dev.h)
     *
     * Every transfer goes through I2C_RDWR so the register address write and the
     * data read happen in one transaction with a repeated start.
     */
    class I2CBus {
    private:
        std::string bus_name;
        int fd = -1;

    public:
        /**
         * @param bus_name Bus name, e.g. "i2c-1" (opens /dev/i2c-1)
         */
        explicit I2CBus(std::string bus_name);
        ~I2CBus();

        I2CBus(I2CBus const&) = delete;
        auto operator=(I2CBus const&) -> I2CBus& = delete;

        /**
         * @brief Open the bus device, throws std::runtime_error on failure
         */
        void open();

        auto write_reg(uint8_t addr, uint8_t reg, uint8_t value) -> bool;
        auto read_regs(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) -> bool;

        [[nodiscard]] auto name() const -> std::string const& { return bus_name; }
    };

} // namespace accel
