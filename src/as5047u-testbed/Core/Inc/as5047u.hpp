#pragma once
#include "stm32g4xx_hal.h"
#include <cstdint>

struct Pin {
  GPIO_TypeDef* port;
  uint16_t      pin;
};

class AS5047U {
public:
  AS5047U(SPI_HandleTypeDef* hspi, Pin cs_pin)
  : m_hspi(hspi), m_cs_pin(cs_pin),
    m_last_position(0), m_velocity(0.0f), m_last_time_us(0) {}

  void init();                           // pull up CS，
  uint16_t read_raw(uint16_t reg);       // read 16-bit reg(no CRC)
  void update_position();                //  up date m_last_position (ANGLECOM)
  void update_velocity();                //  up date m_velocity    (VEL)

  float    get_velocity()  const { return m_velocity; }      // deg/s
  uint16_t get_position()  const { return m_last_position; } // 0..16383

private:
  SPI_HandleTypeDef* m_hspi;
  Pin                m_cs_pin;

  uint16_t m_last_position;
  float    m_velocity;
  uint32_t m_last_time_us;

  void select()   { HAL_GPIO_WritePin(m_cs_pin.port, m_cs_pin.pin, GPIO_PIN_RESET); }
  void unselect() { HAL_GPIO_WritePin(m_cs_pin.port, m_cs_pin.pin, GPIO_PIN_SET);   }

  // no copy
  AS5047U(const AS5047U&)            = delete;
  AS5047U& operator=(const AS5047U&) = delete;
};

// ----------------- AS5047U register map -----------------
namespace AS5047UReg {
  static constexpr uint16_t NOP       = 0x0000;
  static constexpr uint16_t ERRFL     = 0x0001;
  static constexpr uint16_t PROG      = 0x0003;
  static constexpr uint16_t VEL       = 0x3FFC;
  static constexpr uint16_t ANGLEUNC  = 0x3FFE;
  static constexpr uint16_t ANGLECOM  = 0x3FFF;
}
