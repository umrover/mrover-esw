#pragma once

#include <cstdint>
#include <concepts>
#include <numbers>

namespace motors::math {

/**
 * @brief Representation of 3-phase signals (Current or Voltage).
 */
struct PhaseABC { int32_t a; int32_t b; int32_t c; };

/**
 * @brief Representation of 2-axis stationary signals.
 */
struct VectorAlphaBeta { int32_t alpha; int32_t beta; };

/**
 * @brief Representation of 2-axis rotating signals (Torque/Flux).
 */
struct VectorDQ { int32_t d; int32_t q; };

/**
 * @brief STM32G4 CORDIC Driver with integrated FOC transforms.
 */
class Cordic {
public:
    struct Registers {
        volatile uint32_t CSR;
        volatile int32_t  WDATA;
        volatile int32_t  RDATA;
    };

    // Q1.31 constant for 1/sqrt(3)
    static constexpr int32_t INV_SQRT3_Q31 = 1'240'005'187;

    explicit Cordic(uintptr_t base_addr)
        : regs_(*reinterpret_cast<Registers*>(base_addr)) {}

    /**
     * @brief Clarke Transform: (a, b, c) -> (alpha, beta)
     * This is purely algebraic; does not use CORDIC hardware.
     */
    [[nodiscard]] static constexpr VectorAlphaBeta clarke(PhaseABC signals) noexcept {
        // I_alpha = I_a
        // I_beta = (1/sqrt(3)) * (I_a + 2*I_b)
        int64_t beta_long = (static_cast<int64_t>(signals.a) + (static_cast<int64_t>(signals.b) << 1));
        int32_t beta = static_cast<int32_t>((beta_long * INV_SQRT3_Q31) >> 31);

        return { .alpha = signals.a, .beta = beta };
    }

    /**
     * @brief Park Transform: (alpha, beta) -> (d, q) using CORDIC hardware.
     */
    [[nodiscard]] VectorDQ park(VectorAlphaBeta ab, int32_t theta_q31) noexcept {
        prepare_rotation(theta_q31, ab.alpha, ab.beta);

        // Wait for RRDY
        while (!(regs_.CSR & (1 << 31)));

        // G4 CORDIC in rotation mode: first result is X' (d), second is Y' (q)
        return { .d = regs_.RDATA, .q = regs_.RDATA };
    }

    /**
     * @brief Inverse Park Transform: (d, q) -> (alpha, beta) using CORDIC hardware.
     */
    [[nodiscard]] VectorAlphaBeta inverse_park(VectorDQ dq, int32_t theta_q31) noexcept {
        // Inverse Park is a rotation by -theta.
        // In Q31, -theta is simply bitwise negation + 1 or just -theta.
        prepare_rotation(-theta_q31, dq.d, dq.q);

        while (!(regs_.CSR & (1 << 31)));

        return { .alpha = regs_.RDATA, .beta = regs_.RDATA };
    }

    /**
     * @brief Initial hardware setup for FOC rotation mode.
     */
    void init_foc_mode() noexcept {
        regs_.CSR = (0 << 0)   // FUNC: 0 (Cosine/Sine) used for Vector Rotation
                  | (6 << 4)   // PRECISION: 6 iterations (24 cycles)
                  | (1 << 19)  // NRES: 2 results
                  | (1 << 20)  // NARGS: 2 arguments (In rotation mode: Angle and Magnitude)
                  | (1 << 22); // RESSIZE: 32-bit
    }

private:
    /**
     * @brief Internal helper to pipe data to CORDIC.
     * Note: In rotation mode, WDATA sequence is Angle, then X, then Y.
     */
    inline void prepare_rotation(int32_t angle, int32_t x, int32_t y) noexcept {
        regs_.WDATA = angle;
        regs_.WDATA = x;
        regs_.WDATA = y;
    }

    Registers& regs_;
};

} // namespace motors::math