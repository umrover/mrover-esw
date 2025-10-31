/**
 * @file as5047u.cpp
 * @brief Implementation of the AS5047U C++ driver.
 *
 * This file keeps the SPI/CS timing identical to your C version:
 *  - A 16-bit command frame is transmitted with even parity.
 *  - A second 16-bit transfer retrieves the pipelined read data.
 *  - CS is toggled per frame (low before transfer, high after).
 *
 * Notes:
 *  - If your hardware/SPIDMA setup prefers CS low for a burst of two frames,
 *    you can merge the two transfers under one CS window. This variant mirrors
 *    your original (one frame per CS window) for clarity and parity with tests.
 */

#include "as5047u.hpp"

namespace hw {

// ---------- Construction / Init ----------

AS5047U::AS5047U(const AS5047UConfig& cfg) : cfg_(cfg) {}

void AS5047U::init() const {
  // Ensure CS is idle-high (device not selected).
  HAL_GPIO_WritePin(cfg_.cs_port, cfg_.cs_pin, GPIO_PIN_SET);
}

// ---------- Private helpers ----------

inline void AS5047U::cs_low()  const { HAL_GPIO_WritePin(cfg_.cs_port, cfg_.cs_pin, GPIO_PIN_RESET); }
inline void AS5047U::cs_high() const { HAL_GPIO_WritePin(cfg_.cs_port, cfg_.cs_pin, GPIO_PIN_SET); }

inline uint16_t AS5047U::addParity(uint16_t value) {
  // Compute even parity over bits[14:0]; set bit15 if the count is odd.
  uint16_t cnt = 0;
  for (int i = 0; i < 15; ++i) {
    if (value & (1u << i)) { ++cnt; }
  }
  if (cnt & 0x1u) { value |= (1u << 15); }
  return value;
}

// ---------- Public API ----------

uint16_t AS5047U::readReg(uint16_t reg) const {
  // Frame 1: READ command (bit14=1, addr=reg[13:0], bit15=even parity)
  uint16_t cmd = addParity( (1u << 14) | (reg & 0x3FFFu) );
  uint16_t tx = cmd, rx = 0;

  // Transmit command
  cs_low();
  HAL_SPI_TransmitReceive(cfg_.hspi,
                          reinterpret_cast<uint8_t*>(&tx),
                          reinterpret_cast<uint8_t*>(&rx),
                          1 /* 1 x 16-bit word */,
                          HAL_MAX_DELAY);
  cs_high();

  // Frame 2: dummy transfer to clock out the pipelined response
  tx = addParity( (1u << 14) | REG_NOP ); // NOP read keeps R/W=1 to be explicit
  cs_low();
  HAL_SPI_TransmitReceive(cfg_.hspi,
                          reinterpret_cast<uint8_t*>(&tx),
                          reinterpret_cast<uint8_t*>(&rx),
                          1 /* 1 x 16-bit word */,
                          HAL_MAX_DELAY);
  cs_high();

  // The device encodes status bits in the response frame (e.g., error flags).
  // For simplicity (matching your C code), we return only the low 14 data bits.
  return (rx & 0x3FFFu);
}

uint16_t AS5047U::readAngle14() const {
  // REG_ANGLECOM: “compensated” angle; for “raw” use REG_ANGLEUNC.
  return readReg(REG_ANGLECOM) & 0x3FFFu;
}

float AS5047U::readAngleDeg() const {
  const uint16_t raw = readAngle14();             // 0..16383
  return (static_cast<float>(raw) * 360.0f) / 16384.0f;
}

float AS5047U::readVelocityDegPerS() const {
  uint16_t raw = readReg(REG_VEL);

  // The velocity register uses bit13 as the sign bit; perform sign extension.
  if (raw & 0x2000u) {         // if sign bit set
    raw |= 0xC000u;            // extend sign into the upper two bits
  }
  const int16_t vel = static_cast<int16_t>(raw);

  // Keep your established sensitivity: 24.141 deg/s per LSB.
  // (If you later validate a different sensitivity, adjust here centrally.)
  constexpr float kDegPerSecPerLsb = 24.141f;
  return vel * kDegPerSecPerLsb;
}

} // namespace hw
