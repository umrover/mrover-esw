#include "as5047u.hpp"

using namespace AS5047UReg;

static inline uint16_t cmd_read16(uint16_t addr) {
  // 16-bit command frame (no CRC): bit15=0, bit14=1(Read), 13:0=ADDR
  return static_cast<uint16_t>((1u << 14) | (addr & 0x3FFFu));
}

void AS5047U::init() {

  HAL_GPIO_WritePin(m_cs_pin.port, m_cs_pin.pin, GPIO_PIN_SET);

  (void)read_raw(NOP);
}

uint16_t AS5047U::read_raw(uint16_t reg) {
  uint16_t tx = cmd_read16(reg);
  uint16_t rx = 0;

  select();
  HAL_SPI_Transmit(m_hspi, reinterpret_cast<uint8_t*>(&tx), 1, HAL_MAX_DELAY);
  unselect();

  tx = cmd_read16(NOP);
  select();
  HAL_SPI_TransmitReceive(m_hspi,
                          reinterpret_cast<uint8_t*>(&tx),
                          reinterpret_cast<uint8_t*>(&rx),
                          1, HAL_MAX_DELAY);
  unselect();

  return static_cast<uint16_t>(rx & 0x3FFFu);
}

void AS5047U::update_position() {
  m_last_position = read_raw(ANGLECOM); 
}

void AS5047U::update_velocity() {

  uint16_t raw = read_raw(VEL);
 
  if (raw & 0x2000u) raw |= 0xC000u;
  int16_t vel_counts = static_cast<int16_t>(raw);

  // VSens = 24.141 deg/s per LSB
  m_velocity = static_cast<float>(vel_counts) * 24.141f;
}
