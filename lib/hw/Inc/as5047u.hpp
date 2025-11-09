#pragma once
/**
 * @file as5047u.hpp
 * @brief AS5047U magnetic absolute encoder driver (C++ interface).
 *
 * Key protocol notes (AS5047U family):
 *  - Transfers are 16-bit words.
 *  - Bit[15] is the parity bit (even parity over bits[14:0]).
 *  - Bit[14] is R/W (1 = Read, 0 = Write).
 *  - Bits[13:0] are the register address or payload, depending on the frame.
 *  - Reads are “pipelined”: after sending a READ command word, the *data*
 *    becomes available on the *next* frame (a second 16-bit transfer).
 *
 * This driver implements:
 *  - Basic register read with proper pipeline and even parity.
 *  - Angle read (0..16383 → degrees).
 *  - Velocity read converted to deg/s using your existing scale 24.141.
 *
 * Assumptions:
 *  - SPI is already initialized externally.
 *  - CS (chip select) is active-low and controlled via HAL GPIO.
 *  - Timing requirements (setup/hold) are satisfied by SPI + CS toggles.
 *
 * Thread-safety:
 *  - Instances are not thread-safe by themselves. If multiple threads/ISRs
 *    access the same physical sensor or SPI, serialize access externally.
 */

#include <cstdint>


// struct __SPI_HandleTypeDef;                 // HAL里: typedef struct __SPI_HandleTypeDef SPI_HandleTypeDef;
// typedef __SPI_HandleTypeDef SPI_HandleTypeDef;
// struct GPIO_TypeDef;                        // HAL里是一个struct; 前向声明足够用于指针

extern "C" {
#include "stm32g4xx_hal.h" // HAL headers are C; wrap in extern "C" for C++.
}

namespace hw {

/**
 * @brief Static configuration injected at construction time.
 *
 * We keep a small POD config to make the driver easy to mock in tests and
 * reusable across boards. No global handles are referenced.
 */
struct AS5047UConfig {
  SPI_HandleTypeDef* hspi;   ///< HAL SPI handle used for 16-bit transfers.
  GPIO_TypeDef*      cs_port;///< GPIO port for CS pin.
  uint16_t           cs_pin; ///< GPIO pin mask for CS (active-low).
};

/**
 * @brief AS5047U encoder driver.
 *
 * Public API mirrors your original C functions:
 *  - init()
 *  - readReg()
 *  - readAngleDeg()
 *  - readVelocityDegPerS()
 * Plus a convenience method readAngle14() for raw 14-bit angles.
 */
class AS5047U {
public:
  /**
   * @brief Construct with hardware configuration.
   * @param cfg SPI and CS wiring (must outlive this object or remain valid).
   */
  explicit AS5047U(const AS5047UConfig& cfg);

  /**
   * @brief Basic initialization: deassert CS to a safe idle state.
   * Call once after SPI/GPIO are ready.
   */
  void init() const;

  /**
   * @brief Read a 16-bit register (returns the low 14 data bits).
   *
   * Protocol:
   *  1. Send a READ command word (bit14=1, bits[13:0]=addr, bit15=even parity).
   *  2. Perform a second 16-bit transfer (often a NOP read) to receive data.
   *
   * @param reg 14-bit register address.
   * @return uint16_t Data (14 LSBs) from the device.
   */
  uint16_t readReg(uint16_t reg) const;

  /**
   * @brief Read raw 14-bit angle (0..16383).
   * @return Raw angle code.
   */
  uint16_t readAngle14() const;

  /**
   * @brief Read angle converted to degrees.
   * @return Angle in degrees (0..360).
   */
  float readAngleDeg() const;

  /**
   * @brief Read velocity converted to degrees/second.
   *
   * Uses your existing scale factor (24.141 deg/s per LSB).
   * Sign extension is performed using bit13 as the sign bit.
   *
   * @return Velocity in deg/s.
   */
  float readVelocityDegPerS() const;

private:
  // ---- Device register addresses (matching your original C header) ----
  static constexpr uint16_t REG_NOP       = 0x0000;
  static constexpr uint16_t REG_ERRFL     = 0x0001;
  static constexpr uint16_t REG_VEL       = 0x3FFC;
  static constexpr uint16_t REG_ANGLEUNC  = 0x3FFE;
  static constexpr uint16_t REG_ANGLECOM  = 0x3FFF;

  // ---- Helpers ----

  /**
   * @brief Add even parity on bit15 over bits[14:0].
   *
   * Count 1-bits in the lower 15 bits; if odd, set bit15 to produce even parity.
   * @param value Lower 15 bits should already be populated (R/W + address/data).
   * @return Value with parity bit applied in bit15.
   */
  static inline uint16_t addParity(uint16_t value);

  /// Pull CS low (active).
  inline void cs_low() const;
  /// Release CS high (idle).
  inline void cs_high() const;

  AS5047UConfig cfg_;
};

} // namespace hw
