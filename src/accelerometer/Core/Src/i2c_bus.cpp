#include "i2c_bus.hpp"

#include <cerrno>
#include <cstring>
#include <format>
#include <stdexcept>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace accel {

    I2CBus::I2CBus(std::string bus_name) : bus_name(std::move(bus_name)) {}

    I2CBus::~I2CBus() {
        if (fd >= 0) ::close(fd);
    }

    void I2CBus::open() {
        if (fd >= 0) return;
        std::string const path = "/dev/" + bus_name;
        fd = ::open(path.c_str(), O_RDWR);
        if (fd < 0) throw std::runtime_error(std::format("failed to open {}: {}", path, std::strerror(errno)));
    }

    auto I2CBus::write_reg(uint8_t addr, uint8_t reg, uint8_t value) -> bool {
        uint8_t buf[2] = {reg, value};
        i2c_msg msg = {.addr = addr, .flags = 0, .len = 2, .buf = buf};
        i2c_rdwr_ioctl_data xfer = {.msgs = &msg, .nmsgs = 1};
        return ::ioctl(fd, I2C_RDWR, &xfer) >= 0;
    }

    auto I2CBus::read_regs(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) -> bool {
        // write the register address, then read with a repeated start
        i2c_msg msgs[2] = {
                {.addr = addr, .flags = 0, .len = 1, .buf = &reg},
                {.addr = addr, .flags = I2C_M_RD, .len = static_cast<__u16>(len), .buf = data},
        };
        i2c_rdwr_ioctl_data xfer = {.msgs = msgs, .nmsgs = 2};
        return ::ioctl(fd, I2C_RDWR, &xfer) >= 0;
    }

} // namespace accel
