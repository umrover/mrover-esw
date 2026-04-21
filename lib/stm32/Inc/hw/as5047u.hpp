#pragma once

#include <cstdint>
#include <numbers>
#include <span>

#include <hw/pin.hpp>
#include <serial/spi.hpp>
#include <sys.hpp>


namespace mrover {

    namespace as5047u_reg {
        static constexpr uint16_t NOP = 0x0000;
        static constexpr uint16_t ERRFL = 0x0001;
        static constexpr uint16_t VEL = 0x3FFC;
        static constexpr uint16_t ANGLECOM = 0x3FFF;
    } // namespace as5047u_reg

    class AS5047U {
    public:
        static constexpr float CPR = 16384.0f;
        static constexpr float TWO_PI = 2.0f * std::numbers::pi_v<float>;

        AS5047U(SPI* spi, Pin* cs_pin, float const scalar, float const offset)
            : m_spi{spi}, m_cs_pin{cs_pin}, m_scalar{scalar}, m_offset{offset} {
            init();
        }
        AS5047U() = default;

        auto init() -> void {
            if (m_cs_pin) m_cs_pin->set();

            m_init_tx_buf[0] = cmd_read16(as5047u_reg::ERRFL);
            m_init_tx_buf[1] = cmd_read16(as5047u_reg::NOP);

            m_spi->transfer(std::span(&m_init_tx_buf[0], 1), std::span(&m_init_rx_buf[0], 1), nullptr, m_cs_pin);
            m_spi->transfer(std::span(&m_init_tx_buf[1], 1), std::span(&m_init_rx_buf[1], 1), [this]() -> void { this->m_initialized = true; }, m_cs_pin);
        }

        auto set_zero_offset(float const offset) -> void {
            this->m_offset = offset;
        }

        auto update() -> void {
            if (!m_initialized) return;

            m_tx_buf[0] = cmd_read16(as5047u_reg::ANGLECOM);
            m_tx_buf[1] = cmd_read16(as5047u_reg::NOP);

            m_spi->transfer(std::span(&m_tx_buf[0], 1), std::span(&m_rx_buf[0], 1), nullptr, m_cs_pin);
            m_spi->transfer(std::span(&m_tx_buf[1], 1), std::span(&m_rx_buf[1], 1), [this]() -> void {
                uint16_t const new_pos = m_rx_buf[1] & 0x3FFFu;
                uint32_t const now = System::get_ticks();

                if (!this->m_first_read_done) {
                    this->m_raw_pos = new_pos;
                    this->m_last_tick = now;
                    this->m_first_read_done = true;
                    return;
                }

                if (float const dt = static_cast<float>(now - this->m_last_tick) / 1000.0f; dt > 0.0f) {
                    int32_t delta = static_cast<int32_t>(new_pos) - static_cast<int32_t>(this->m_raw_pos);
                    if (delta > 8192) delta -= 16384;
                    if (delta < -8192) delta += 16384;
                    float const rads_per_tick = (static_cast<float>(delta) / CPR) * TWO_PI;

                    this->m_velocity_rads = rads_per_tick / dt;
                }

                this->m_raw_pos = new_pos;
                this->m_last_tick = now; }, m_cs_pin);
        }

        [[nodiscard]] auto get_position() const -> float {
            float const rads = (static_cast<float>(m_raw_pos) / CPR) * TWO_PI;
            return (rads * m_scalar) - m_offset;
        }

        [[nodiscard]] auto get_velocity() const -> float {
            return m_velocity_rads * m_scalar;
        }

    private:
        SPI* m_spi{};
        Pin* m_cs_pin{};
        float m_scalar{};
        float m_offset{};

        uint16_t m_raw_pos{0};
        float m_velocity_rads{0.0f};
        uint32_t m_last_tick{0};

        bool volatile m_initialized{false};
        bool volatile m_first_read_done{false};

        uint16_t m_init_tx_buf[2]{0};
        uint16_t m_init_rx_buf[2]{0};

        uint16_t m_tx_buf[2]{0};
        uint16_t m_rx_buf[2]{0};

        static auto cmd_read16(uint16_t const addr) -> uint16_t {
            auto cmd = static_cast<uint16_t>((1u << 14) | (addr & 0x3FFFu));
            uint16_t parity = 0;
            uint16_t temp = cmd;
            while (temp > 0) {
                parity ^= (temp & 1);
                temp >>= 1;
            }
            if (parity) cmd |= (1u << 15);
            return cmd;
        }
    };
} // namespace mrover
