#include "main.h"
#include "hw/flash.hpp"
#include "stm32g4xx_hal_gpio.h"
#include <cstdio>

extern "C" {
    #include <stdio.h>
}

namespace mrover {

  Flash flash;
  int count;
  uint32_t start_addr;
  uint32_t curr_addr;
  uint64_t read = 0;
  float pi = 3.14159;
  float read_float = 0.0;
  auto init() -> void {
    count = 1;
    start_addr = (uint32_t)flash.info.start;
    // printf("Init start_addr: %lx\n\r", start_addr);
    curr_addr = start_addr;
    // printf("Init curr_addr: %lx\n\r", curr_addr);
    printf("PI is : %f\n\r", pi);
    flash.erase();
  }

  auto loop() -> void {
    // printf("start of loop %lx\n\r",curr_addr);
    while(true) {
      
      int button_state = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
      /*
      if (button_state && count < 256) {
        // printf("before write addr: %lx\n\r", curr_addr);

        flash.program_double_word(curr_addr,  (uint64_t)(count * 2));
        // printf("after write addr: %lx\n\r", curr_addr);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);        
        // printf("before read addr: %lx\n\r", curr_addr);
        read = flash.read_double_word(curr_addr);
        // printf("after read addr: 0x%lx\n\r", curr_addr);
        printf("Count: %d, Read: %lu, Addr: 0x%lx\n\r", 
          count, 
          (uint32_t)read, 
          (uint32_t)curr_addr);
        curr_addr += 8;
        ++count;
      }
        */
      if (button_state && count < 256) {
        flash.write_float(curr_addr, (pi + count));

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        
        read_float = flash.read_float(curr_addr);
        printf("Read Float: %f\n\r", read_float);
        ++count;
        curr_addr += 8;

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