#include "mpu6050.hpp"

#include <chrono>
#include <thread>

namespace accel {

    namespace {
        // Registers (RM-MPU-6000A rev 4.2)
        constexpr uint8_t CONFIG = 0x1A;       // DLPF_CFG, page 13
        constexpr uint8_t GYRO_CONFIG = 0x1B;  // FS_SEL, page 14
        constexpr uint8_t ACCEL_CONFIG = 0x1C; // AFS_SEL, page 15
        constexpr uint8_t ACCEL_XOUT_H = 0x3B; // accel(6) temp(2) gyro(6), page 29
        constexpr uint8_t PWR_MGMT_1 = 0x6B;   // page 40
        constexpr uint8_t WHO_AM_I = 0x75;     // page 45

        constexpr uint8_t WHO_AM_I_VALUE = 0x68;

        // AFS_SEL = 0 (±2 g), FS_SEL = 0 (±250 dps)
        constexpr double ACCEL_LSB_PER_G = 16384.0;
        constexpr double GYRO_LSB_PER_DPS = 131.0;

        auto be16(uint8_t const* p) -> int16_t { return static_cast<int16_t>((p[0] << 8) | p[1]); }

        auto now_ms() -> long long {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                    .count();
        }
    } // namespace

    MPU6050::MPU6050(I2CBus& bus, uint8_t addr) : bus(bus), addr(addr) {}

    auto MPU6050::init() -> bool {
        uint8_t id = 0;
        if (!bus.read_regs(addr, WHO_AM_I, &id, 1) || id != WHO_AM_I_VALUE) return false;

        // device reset, then wake with PLL on gyro X as clock source
        if (!bus.write_reg(addr, PWR_MGMT_1, 0x80)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!bus.write_reg(addr, PWR_MGMT_1, 0x01)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        return bus.write_reg(addr, CONFIG, 0x03) &&
               bus.write_reg(addr, GYRO_CONFIG, 0x00) &&
               bus.write_reg(addr, ACCEL_CONFIG, 0x00);
    }

    auto MPU6050::read(ImuSample& out) -> bool {
        uint8_t b[14];
        if (!bus.read_regs(addr, ACCEL_XOUT_H, b, sizeof(b))) return false;

        out.time = now_ms();
        out.accel = {be16(b) / ACCEL_LSB_PER_G, be16(b + 2) / ACCEL_LSB_PER_G, be16(b + 4) / ACCEL_LSB_PER_G};
        // b[6..7] is temperature, skipped
        out.gyro = {be16(b + 8) / GYRO_LSB_PER_DPS, be16(b + 10) / GYRO_LSB_PER_DPS, be16(b + 12) / GYRO_LSB_PER_DPS};
        return true;
    }

} // namespace accel
