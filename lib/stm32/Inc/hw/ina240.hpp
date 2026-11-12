#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

#include <adc.hpp>
#include <logger.hpp>
#include <sys.hpp>

namespace mrover {
#ifdef HAL_ADC_MODULE_ENABLED
    /**
     * Wrapper for INA240 current sensor through an STM32 ADC channel.
     *
     * REF1/REF2 at VS/GND, zero current at VS/2. VS == ADC ref.
     *
     * I = (counts - zero) * vref / full_scale / (gain * shunt)
     *
     * From testing:
     * - oversample as much as possible (256x on STM32G4)
     * - select gain & shunt to avoid clipping
     */
    class INA240 {
    public:
        // INA240 gain is set per IC (INA240A1-INA240A4)
        enum class gain_t : uint8_t {
            A1 = 20,
            A2 = 50,
            A3 = 100,
            A4 = 200,
        };

        struct options_t {
            gain_t gain;              // individual IC gain
            float shunt_ohms;         // shunt value (Ohms)
            float vref;               // ADC ref (Volts)
            float full_scale;         // max ADC result, e.g. 4095 * 256 >> 4 = 65520 with 256x oversampling
            uint32_t cal_settle_ms;   // pre-calibration settling time (wait for rails to ramp)
            uint16_t cal_samples;     // readings per attempt
            uint16_t cal_attempts;    // 0 value will retry until success
            uint32_t cal_retry_ms;    // wait between attempts
            float midpoint_tolerance; // allowed |zero - full_scale / 2|, fraction of full scale
            float cal_max_spread_a;   // calibration fails if max-min readings exceed this
        };

        enum class status_t : uint8_t {
            UNCALIBRATED,
            OK,
            BAD_MIDPOINT, // zero far from VS/2
            NOT_IDLE,     // readings moved significantly during calibration
            ADC_ERROR,    // conversion failed
        };

        /**
         * Constructor
         *
         * @param adc ADC object to read sensor
         * @param position ADC channel position
         */
        INA240(ADC* adc, size_t const position, options_t const& options)
            : m_adc{adc}, m_position{position}, m_options{options},
              m_amps_per_count{amps_per_count(options)},
              m_zero{options.full_scale / 2.0f},
              // linear output range: GND + 50 mV to VS - 200 mV
              m_low{0.05f / options.vref * options.full_scale},
              m_high{(options.vref - 0.2f) / options.vref * options.full_scale} {}

        /**
         * Zero-current calibration sequence.
         *
         * Attempt several calibration sequences while load idle.
         */
        auto calibrate() -> status_t {
            System::delay_ms(m_options.cal_settle_ms);
            for (uint16_t attempt = 1;; ++attempt) {
                m_status = calibrate_once();
                if (m_status == status_t::OK) break;
                Logger::instance().warn("ina240 calibration attempt %u: %s (mean %u, expected ~%u)",
                                        attempt, status_name(m_status), m_raw, static_cast<uint32_t>(m_options.full_scale / 2.0f));
                if (attempt == m_options.cal_attempts) break;
                System::delay_ms(m_options.cal_retry_ms);
            }
            return m_status;
        }

        /**
         * Take one reading.
         *
         * @return current sample in amps
         */
        auto update() -> float {
            m_raw = m_adc->read(m_position);
            m_saturated = m_raw < m_low || m_raw > m_high;
            m_current = (static_cast<float>(m_raw) - m_zero) * m_amps_per_count;
            return m_current;
        }

        [[nodiscard]] auto current() const -> float { return m_current; }
        [[nodiscard]] auto raw() const -> uint32_t { return m_raw; }
        [[nodiscard]] auto zero() const -> float { return m_zero; }
        [[nodiscard]] auto status() const -> status_t { return m_status; }
        [[nodiscard]] auto options() const -> options_t const& { return m_options; }
        [[nodiscard]] auto saturated() const -> bool { return m_saturated; }
        [[nodiscard]] auto range() const -> float { return std::min(m_high - m_zero, m_zero - m_low) * m_amps_per_count; }

        [[nodiscard]] static constexpr auto status_name(status_t const s) -> char const* {
            switch (s) {
                case status_t::UNCALIBRATED:
                    return "uncalibrated";
                case status_t::OK:
                    return "ok";
                case status_t::BAD_MIDPOINT:
                    return "bad midpoint";
                case status_t::NOT_IDLE:
                    return "not idle";
                case status_t::ADC_ERROR:
                    return "adc error";
            }
            return "?";
        }

        [[nodiscard]] static constexpr auto amps_per_count(options_t const& o) -> float {
            return o.vref / o.full_scale / (static_cast<float>(std::to_underlying(o.gain)) * o.shunt_ohms);
        }

        [[nodiscard]] static constexpr auto check_baseline(options_t const& o, float const mean, float const lo, float const hi) -> status_t {
            float const off = mean - o.full_scale / 2.0f;
            if ((off < 0 ? -off : off) > o.midpoint_tolerance * o.full_scale) return status_t::BAD_MIDPOINT;
            if ((hi - lo) * amps_per_count(o) > o.cal_max_spread_a) return status_t::NOT_IDLE;
            return status_t::OK;
        }

    private:
        auto calibrate_once() -> status_t {
            float sum{}, lo{m_options.full_scale}, hi{};
            for (uint16_t i = 0; i < m_options.cal_samples; ++i) {
                if (!m_adc->sample()) return status_t::ADC_ERROR;
                auto const raw = static_cast<float>(m_adc->value(m_position));
                sum += raw;
                lo = std::min(lo, raw);
                hi = std::max(hi, raw);
            }
            float const mean = sum / m_options.cal_samples;
            m_raw = static_cast<uint32_t>(mean);
            status_t const s = check_baseline(m_options, mean, lo, hi);
            if (s == status_t::OK) m_zero = mean;
            return s;
        }

        ADC* m_adc;
        size_t m_position;
        options_t m_options;
        float m_amps_per_count;
        float m_zero;
        float m_low;
        float m_high;
        uint32_t m_raw{};
        float m_current{};
        bool m_saturated{};
        status_t m_status{status_t::UNCALIBRATED};
    };

#else  // HAL_ADC_MODULE_ENABLED
    class __attribute__((unavailable("enable 'ADC' in STM32CubeMX to use mrover::INA240"))) INA240 {
    public:
        template<typename... Args>
        explicit INA240(Args&&... args) {}
    };
#endif // HAL_ADC_MODULE_ENABLED
} // namespace mrover
