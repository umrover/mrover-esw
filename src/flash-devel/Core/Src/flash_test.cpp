#include "main.h"
#include "hw/flash.hpp"
#include "stm32g4xx_hal_gpio.h"
#include <cstdio>
#include <variant>

extern "C" {
    #include <stdio.h>
}

namespace mrover {

  Flash flash(0x0801F800);
  int count;
  uint32_t start_addr;
  uint32_t curr_addr;
  uint64_t read = 0;
  float pi = 3.14159;
  float read_float = 0.0;
  auto init() -> void {
    printf("===== RESET =====\n\r");
    count = 1;
    start_addr = (uint32_t)flash.info.start;
    // printf("Init start_addr: %lx\n\r", start_addr);
    curr_addr = start_addr;
    // printf("Init curr_addr: %lx\n\r", curr_addr);
    
    flash.erase();
    uint64_t combined =  0;
    combined |= ((uint64_t)8 << 56) | ((uint64_t)8 << 48) | ((uint64_t)16 << 32) | ((uint64_t)32 << 0);
    flash.write(0x00, 8, combined);
    flash.write(0x08, 2, (uint16_t)(16));
    flash.write(0x10, 1, (uint8_t)(8));
    flash.write(0x18, 1, (uint8_t)(8));
    printf("====== WROTE: 32 at 0x00, 16 at 0x04, 8 at 0x06, 8 at 0x07 ======\n\r");
  
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

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        
        curr_addr = 0x00;
        uint32_t read_data_32 = flash.read<uint32_t>(curr_addr);
        printf("Read at address 0x00: %lu\n\r", read_data_32);
        curr_addr = 0x04;
        uint16_t read_data_16 = flash.read<uint16_t>(curr_addr);
        printf("Read at address 0x04: %u\n\r", read_data_16);
        curr_addr = 0x06;
        uint8_t read_data_8 = flash.read<uint8_t>(curr_addr);
        printf("Read at address 0x06: %u\n\r", read_data_8);
        curr_addr = 0x07;
        read_data_8 = flash.read<uint8_t>(curr_addr);
        printf("Read at address 0x07: %u\n\r", read_data_8);
        read_data_16 = flash.read<uint16_t>(0x08);
        printf("Read at address 0x08: %u\n\r", read_data_16);
        read_data_8 = flash.read<uint8_t>(0x10);
        printf("Read at address 0x10: %u\n\r", read_data_8);
        read_data_8 = flash.read<uint8_t>(0x18);
        printf("Read at address 0x18: %u\n\r", read_data_8);
        printf("====BYTE READING TEST====\n\r");
        read_data_8 = flash.read_byte(start_addr + 0x10);
        printf("Read at byte at address 0x10: %u\n\r", read_data_8);

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