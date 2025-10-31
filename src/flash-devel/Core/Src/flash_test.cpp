#include "main.h"
#include "hw/flash.hpp"
#include "stm32g4xx_hal_gpio.h"


namespace mrover {

  Flash flash;
  int count;
  uint32_t start_addr;
  uint32_t curr_addr;
  uint64_t read = 0;
  auto init() -> void {
    count = 1;
    flash.get_info();
    start_addr = (uint32_t)flash.info.start;
    curr_addr = start_addr;
  }

  auto loop() -> void {

    while(true) {
      
      int button_state = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
      if (button_state && count < 32) {
        flash.program_word(curr_addr, count);
        ++count;
        flash.read_word(curr_addr, 1);
        curr_addr += 64;
      }
      

    }

  }

}


extern "C" {
  void PostInit() {
    mrover::init();
  }

  void Loop() {
    mrover::loop();
  }
}