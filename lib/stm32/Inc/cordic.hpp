#pragma once

#include <cstdint>
#include <numbers>

#ifdef STM32
#include "main.h"
#endif // STM32

namespace mrover {

    // abc: the three phase vals (currents or voltages), on axes 120 deg apart, a-axis at 0
    struct abc_t {
        float a;
        float b;
        float c;
    };

    // alpha-beta: stationary and orthogonal, alpha lies on the phase-a axis, beta leads it by 90 deg
    struct alpha_beta_t {
        float alpha;
        float beta;
    };

    // dq: rotating with the rotor at electrical theta (measured from alpha)
    // d lies on the rotor flux (magnet) axis, q leads d by 90
    // torque is proportional to i_q; i_d sets the flux (target 0)
    struct dq_t {
        float d;
        float q;
    };

    // sincos: sine and cosine representation of theta
    struct sincos_t {
        float sin;
        float cos;
    };

    /**
     * COordinate Rotation Digital Integrated Circuit (CORDIC)
     *
     * Driver for STM32G4 Cordic designed for three-phase FOC
     * ref: https://www.ti.com/lit/an/bpra048/bpra048.pdf
     */
    class CORDICDriver {
    public:
        /**
         * Clarke transform: three-phase (a, b, c) -> stationary orthogonal (alpha, beta)
         */
        static constexpr auto clarke(abc_t const& x) -> alpha_beta_t {
            constexpr float inv_sqrt3 = 1.0f / std::numbers::sqrt3_v<float>;
            return {(2.0f * x.a - x.b - x.c) * (1.0f / 3.0f), (x.b - x.c) * inv_sqrt3};
        }

        /**
         * Inverse Clarke transform: stationary (alpha, beta) -> three-phase (a, b, c), zero-sequence = 0
         */
        static constexpr auto inv_clarke(alpha_beta_t const& x) -> abc_t {
            constexpr float half_sqrt3 = std::numbers::sqrt3_v<float> / 2.0f;
            return {x.alpha, -0.5f * x.alpha + half_sqrt3 * x.beta, -0.5f * x.alpha - half_sqrt3 * x.beta};
        }

        /**
         * Park transform: stationary (alpha, beta) -> rotor-aligned (d, q) at electrical theta
         */
        static constexpr auto park(alpha_beta_t const& x, sincos_t const& t) -> dq_t {
            return {x.alpha * t.cos + x.beta * t.sin, -x.alpha * t.sin + x.beta * t.cos};
        }

        /**
         * Inverse Park transform: rotor-aligned (d, q) -> stationary (alpha, beta), rotation by +theta
         */
        static constexpr auto inv_park(dq_t const& x, sincos_t const& t) -> alpha_beta_t {
            return {x.d * t.cos - x.q * t.sin, x.d * t.sin + x.q * t.cos};
        }

        /**
         * Convert angle in radians to the CORDIC angle argument (theta / pi in q1.31, [-1, 1)
         * for [-pi, pi), wrap angles outside [-pi, pi)
         */
        static constexpr auto rad2cordic(float const rad) -> int32_t {
            // x = theta / pi, wrapped to [-1, 1)
            float x = rad * std::numbers::inv_pi_v<float>;
            if (x >= 1.0f || x < -1.0f) {
                float const half = x * 0.5f;
                x -= 2.0f * static_cast<float>(static_cast<int32_t>(half + (half >= 0.0f ? 0.5f : -0.5f)));
                if (x >= 1.0f) x -= 2.0f;
                if (x < -1.0f) x += 2.0f;
            }
            // |x| < 1, so x * 2^31 rounds to at most 2147483520 as a float, no overflow
            return static_cast<int32_t>(x * 2147483648.0f);
        }

        /**
         * Convert CORDIC angle representation (theta / pi in q1.31) to radians
         */
        static constexpr auto cordic2rad(int32_t const q) -> float {
            return static_cast<float>(q) * (1.0f / 2147483648.0f);
        }

#ifdef HAL_CORDIC_MODULE_ENABLED
        CORDICDriver() = default;

        explicit CORDICDriver(CORDIC_HandleTypeDef* hcordic) : m_hcordic{hcordic} {}

        /**
         * sin and cos of theta (radians) on the CORDIC
         */
        [[nodiscard]] auto sincos(float const rad) const -> sincos_t {
            int32_t const angle = rad2cordic(rad);
            CORDIC_TypeDef* const regs = m_hcordic->Instance;

            uint32_t const primask = __get_PRIMASK();
            __disable_irq();
            regs->CSR = CSR_COSINE;
            regs->WDATA = static_cast<uint32_t>(angle); // ARG1: theta / pi
            regs->WDATA = MODULUS_ONE; // ARG2: modulus m = 1
            auto const cos = static_cast<int32_t>(regs->RDATA); // RES1: m cos(theta)
            auto const sin = static_cast<int32_t>(regs->RDATA); // RES2: m sin(theta)
            __set_PRIMASK(primask);

            return {cordic2rad(sin), cordic2rad(cos)};
        }

    private:
        // set op to cos, 24 iter (precision=6), scale=0, nargs=2, nres=2
        static constexpr uint32_t CSR_COSINE = (6U << CORDIC_CSR_PRECISION_Pos) | CORDIC_CSR_NARGS | CORDIC_CSR_NRES;
        static constexpr uint32_t MODULUS_ONE = 0x7FFFFFFFU;

        CORDIC_HandleTypeDef* m_hcordic{};
#endif // HAL_CORDIC_MODULE_ENABLED
    };

} // namespace mrover
