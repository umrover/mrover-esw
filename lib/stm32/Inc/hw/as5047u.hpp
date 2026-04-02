#pragma once

#include <serial/spi.hpp>

namespace mrover {

    namespace as5047u_reg {
        static constexpr uint16_t NOP = 0x0000;
        static constexpr uint16_t ERRFL = 0x0001;
        static constexpr uint16_t SETTINGS1 = 0x0018;
        static constexpr uint16_t ZPOSM = 0x0016; // Zero Position MSB
        static constexpr uint16_t ZPOSL = 0x0017; // Zero Position LSB
        static constexpr uint16_t DIAAGC = 0x3FFD;
        static constexpr uint16_t VEL = 0x3FC0;
        static constexpr uint16_t ANGLECOM = 0x3FFF;
    } // namespace as5047u_reg

    class AS5047U {
    public:
        AS5047U(SPI* spi, float const scalar, float const offset, uint8_t const noise_margin, const std::function<void()>& on_complete = nullptr) : m_spi{spi}, m_scalar{scalar}, m_offset{offset} {
            init(noise_margin, on_complete);
        }
        AS5047U() = default;

        auto init(uint8_t const noise_margin = 0, const std::function<void()>& on_complete = nullptr) -> void {
            m_tx_buf[0] = make_cmd(as5047u_reg::ERRFL);
            m_tx_buf[1] = make_cmd(as5047u_reg::NOP);

            m_spi->transfer(std::span(m_tx_buf, 2), std::span(m_rx_buf, 2), [this, noise_margin, on_complete]() -> void {
                // write to SETTINGS1
                uint16_t const cmd = make_cmd(as5047u_reg::SETTINGS1, false);
                this->m_tx_buf[0] = cmd;
                this->m_tx_buf[1] = make_cmd(noise_margin & 0x03, false);

                this->m_spi->transfer(std::span(this->m_tx_buf, 2), std::span(this->m_rx_buf, 2), on_complete);
            });
        }

        auto set_zero_offset(float const offset) -> void {
            this->m_offset = offset;
        }

        auto set_zero_on_chip(const std::function<void()>& on_complete = nullptr) -> void {
            m_tx_buf[0] = make_cmd(as5047u_reg::ANGLECOM);
            m_tx_buf[1] = make_cmd(as5047u_reg::NOP);

            m_spi->transfer(std::span(m_tx_buf, 2), std::span(m_rx_buf, 2), [this, on_complete]() -> void {
                uint16_t const current_angle = this->m_rx_buf[1] & 0x3FFF;

                // write zero pos MSB addr & data
                this->m_tx_large_buf[0] = make_cmd(as5047u_reg::ZPOSM, false);
                this->m_tx_large_buf[1] = make_cmd((current_angle >> 8) & 0x3F, false);
                // write zero pos LSB addr & data
                this->m_tx_large_buf[2] = make_cmd(as5047u_reg::ZPOSL, false);
                this->m_tx_large_buf[3] = make_cmd(current_angle & 0xFF, false);

                this->m_spi->transfer(std::span(this->m_tx_large_buf, 4), std::span(this->m_rx_large_buf, 4), on_complete);
            });
        }

        auto update() -> void {
            m_tx_buf[0] = make_cmd(as5047u_reg::ANGLECOM);
            m_tx_buf[1] = make_cmd(as5047u_reg::VEL);
            m_tx_buf[2] = make_cmd(as5047u_reg::NOP);

            m_spi->transfer(std::span(m_tx_buf, 3), std::span(m_rx_buf, 3), [this]() -> void {
                this->m_raw_pos = m_rx_buf[1] & 0x3FFF;

                uint16_t raw_vel = m_rx_buf[2] & 0x3FFF;
                if (raw_vel & 0x2000) raw_vel |= 0xC000;
                this->m_raw_vel = static_cast<int16_t>(raw_vel);
            });
        }

        [[nodiscard]] auto get_position() const -> float {
            return static_cast<float>(m_raw_pos) * m_scalar - m_offset;
        }

        [[nodiscard]] auto get_velocity() const -> float {
            return static_cast<float>(m_raw_vel) * m_scalar;
        }

        auto set_noise_margin(uint8_t const margin, std::function<void()> const& on_complete = nullptr) -> void {
            uint16_t const cmd = make_cmd(as5047u_reg::SETTINGS1, false);
            this->m_tx_buf[0] = cmd;
            this->m_tx_buf[1] = make_cmd(margin & 0x03, false);

            this->m_spi->transfer(std::span(this->m_tx_buf, 2), std::span(this->m_rx_buf, 2), on_complete);
        }

    private:
        SPI* m_spi{};
        float m_scalar{};
        float m_offset{};
        uint16_t m_raw_pos{0};
        int16_t m_raw_vel{0};

        // TODO(eric): RTOS: ensure these aren't used concurrently
        uint16_t m_tx_buf[3]{0};
        uint16_t m_rx_buf[3]{0};
        uint16_t m_tx_large_buf[4]{0};
        uint16_t m_rx_large_buf[4]{0};

        static auto make_cmd(uint16_t const addr, bool const is_read = true) -> uint16_t {
            uint16_t cmd = (addr & 0x3FFF);
            if (is_read) cmd |= (1u << 14);

            // parity calculation
            uint16_t p = 0;
            for (uint8_t i = 0; i < 15; ++i)
                if (cmd >> i & 1) ++p;
            if (p % 2 == 0) cmd |= 1u << 15;
            return cmd;
        }
    };
} // namespace mrover
