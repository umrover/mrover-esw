#pragma once

#include <cstdint>
#include <span>

#include <serial/spi.hpp>
#include <hw/pin.hpp>
#include <logger.hpp>

namespace mrover {

    namespace as5047u_reg {
        static constexpr uint16_t NOP = 0x0000;
        static constexpr uint16_t ERRFL = 0x0001;
        static constexpr uint16_t VEL = 0x3FC0;
        static constexpr uint16_t ANGLECOM = 0x3FFF;
    } // namespace as5047u_reg

    class AS5047U {
    public:
        AS5047U(SPI* spi, Pin* cs_pin, float const scalar, float const offset, uint8_t const noise_margin)
            : m_spi{spi}, m_cs_pin{cs_pin}, m_scalar{scalar}, m_offset{offset} {
            init(noise_margin);
        }
        AS5047U() = default;

        auto init(uint8_t const noise_margin = 0) -> void {
            m_cs_pin->set(); // Ensure CS is pulled high initially
            read_reg(as5047u_reg::ERRFL); // Clear any startup errors
        }

        auto set_zero_offset(float const offset) -> void {
            this->m_offset = offset;
        }

        auto update() -> void {
            // Read ANGLECOM via pipeline
            this->m_raw_pos = read_reg(as5047u_reg::ANGLECOM);

            // Read VEL via pipeline
            uint16_t raw_vel = read_reg(as5047u_reg::VEL);

            // Handle negative velocities (14-bit sign extension)
            if (raw_vel & 0x2000u) raw_vel |= 0xC000u;
            this->m_raw_vel = static_cast<int16_t>(raw_vel);

            Logger::instance().info("pos: %u", this->m_raw_pos);
            Logger::instance().info("vel: %d", this->m_raw_vel);
        }

        [[nodiscard]] auto get_position() const -> float {
            return static_cast<float>(m_raw_pos) * m_scalar - m_offset;
        }

        [[nodiscard]] auto get_velocity() const -> float {
            // Your older file used 24.141f scalar for velocity, but we use the configurable m_scalar here
            return static_cast<float>(m_raw_vel) * m_scalar;
        }

    private:
        SPI* m_spi{};
        Pin* m_cs_pin{};
        float m_scalar{};
        float m_offset{};
        uint16_t m_raw_pos{0};
        int16_t m_raw_vel{0};

        static inline auto cmd_read16(uint16_t addr) -> uint16_t {
            // Base command: bit14=1 (Read), 13:0=ADDR
            uint16_t cmd = static_cast<uint16_t>((1u << 14) | (addr & 0x3FFFu));

            // Calculate even parity
            uint16_t parity = 0;
            uint16_t temp = cmd;
            while (temp > 0) {
                parity ^= (temp & 1);
                temp >>= 1;
            }

            // If the number of 1s is odd (parity == 1), set Bit 15 to make it even
            if (parity) {
                cmd |= (1u << 15);
            }

            return cmd;
        }

        // Implements the pipeline read that "used to work"
        auto read_reg(uint16_t const reg) -> uint16_t {
            uint16_t tx = cmd_read16(reg);
            uint16_t rx = 0;

            // Step 1: Send the requested address
            m_cs_pin->reset();
            // m_spi->transfer(std::span(&tx, 1), std::span(&rx, 1));
            HAL_SPI_Transmit(m_spi->handle(), reinterpret_cast<uint8_t*>(&tx), 1, HAL_MAX_DELAY);
            m_cs_pin->set();

            // Small delay to meet CSn high time requirement between frames (t_CSn > 350ns)
            for(volatile int i = 0; i < 15; ++i) {}

            // Step 2: Send NOP and clock in the actual data from the previous request
            tx = cmd_read16(as5047u_reg::NOP);
            m_cs_pin->reset();
            HAL_SPI_TransmitReceive(m_spi->handle(), reinterpret_cast<uint8_t*>(&tx), reinterpret_cast<uint8_t*>(&rx), 1, HAL_MAX_DELAY);
            // m_spi->transfer(std::span(&tx, 1), std::span(&rx, 1));
            m_cs_pin->set();

            for(volatile int i = 0; i < 15; ++i) {}

            return rx & 0x3FFFu;
        }
    };
} // namespace mrover