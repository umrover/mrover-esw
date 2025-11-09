#include "as5047u.hpp"

using namespace AS5047UReg;

static inline uint16_t cmd_read16(uint16_t addr) {
  // 16-bit command frame (no CRC): bit15=0, bit14=1(Read), 13:0=ADDR
  return static_cast<uint16_t>((1u << 14) | (addr & 0x3FFFu));
}

void AS5047U::init() {
  // CS 默认拉高
  HAL_GPIO_WritePin(m_cs_pin.port, m_cs_pin.pin, GPIO_PIN_SET);

  // 可选：清一次读管线
  (void)read_raw(NOP);
}

uint16_t AS5047U::read_raw(uint16_t reg) {
  // 发送命令帧
  uint16_t tx = cmd_read16(reg);
  uint16_t rx = 0;

  select();
  HAL_SPI_Transmit(m_hspi, reinterpret_cast<uint8_t*>(&tx), 1, HAL_MAX_DELAY);
  unselect();

  // 下一个帧读回数据（发送一个 NOP 触发返回）
  tx = cmd_read16(NOP);
  select();
  HAL_SPI_TransmitReceive(m_hspi,
                          reinterpret_cast<uint8_t*>(&tx),
                          reinterpret_cast<uint8_t*>(&rx),
                          1, HAL_MAX_DELAY);
  unselect();

  // rx 的 bit15=Warn, bit14=Error, bit13:0=DATA（无CRC模式）
  // 如需调试，可在此检查 (rx & 0xC000) 标志位
  return static_cast<uint16_t>(rx & 0x3FFFu);
}

void AS5047U::update_position() {
  m_last_position = read_raw(ANGLECOM);  // 已含 DAEC 的角度
  // 0..16383 对应 0..360°；若你想要弧度或度，外部换算即可
}

void AS5047U::update_velocity() {
  // VEL 为 14-bit 二补码，单位见 VSens
  uint16_t raw = read_raw(VEL);
  // 符号扩展到 16bit
  if (raw & 0x2000u) raw |= 0xC000u;
  int16_t vel_counts = static_cast<int16_t>(raw);

  // VSens = 24.141 deg/s per LSB（典型值）
  m_velocity = static_cast<float>(vel_counts) * 24.141f;
}
