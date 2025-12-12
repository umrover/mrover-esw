#include "main.h"
#include "hw/flash.hpp"
#include "stm32g4xx_hal_gpio.h"

extern "C" {
    #include <stdio.h>
}

namespace mrover {

  struct bmc_config_t {
    static constexpr reg_t<uint8_t> CAN_ID{"can_id", 0x00};
    static constexpr reg_t<uint8_t> LIMITS_ENABLED{"limits_enabled", 0x01};
    static constexpr reg_t<uint16_t> INT_VALUE{"int_value", 0x02};
    static constexpr reg_t<float> FLOAT_VALUE{"float_value", 0x04};

    static constexpr auto all() {
        return std::make_tuple(CAN_ID, LIMITS_ENABLED, INT_VALUE, FLOAT_VALUE);
    }

    static consteval uint16_t size_bytes() {
        return validated_config_t<bmc_config_t>::size_bytes();
    }
  };

  Flash<bmc_config_t> flash(LAST_PAGE_START);
  // FlashPageManager mgr(flash);
  int count;
  uint32_t start_addr;
  uint32_t curr_addr;
  uint64_t read = 0;

  uint32_t read_32;
  uint16_t read_16;
  uint8_t read_8;

  auto init() -> void {
    printf("===== RESET =====\n\r");
    count = 1;
    start_addr = (uint32_t)flash.info.start;
    // printf("Init start_addr: %lx\n\r", start_addr);
    curr_addr = start_addr;
    // printf("Init curr_addr: %lx\n\r", curr_addr);

    //flash.write_config(bmc_config_t::CAN_ID, 0x02);
    auto const id = flash.read_config(bmc_config_t::CAN_ID);
    printf("CAN ID: 0x%u \n\r", id);
    //flash.write_config(bmc_config_t::LIMITS_ENABLED, 0x20);
    auto const lims_enabled = flash.read_config(bmc_config_t::LIMITS_ENABLED);
    printf("LIMITS ENABLED: 0x%u \n\r", lims_enabled);
    //flash.write_config(bmc_config_t::INT_VALUE, 12);
    auto const int_value = flash.read_config(bmc_config_t::INT_VALUE);
    printf("INT VALUE : %u \n\r", int_value);
    //flash.write_config(bmc_config_t::FLOAT_VALUE, 3.1);
    auto const float_value = flash.read_config(bmc_config_t::FLOAT_VALUE);
    printf("FLOAT VALUE : %.5f \n\r", float_value);

  }
  
  auto loop() -> void {
    // printf("start of loop %lx\n\r",curr_addr);
    printf("====== LOOP ======\n\r");
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
        
        
        read_32 = flash.read<uint32_t>(0x00);
        read_16 = flash.read<uint16_t>(0x04);
        printf("=== READ %lu at 0x%lx ===\n\r", read_32, (uint32_t)0x00);
        printf("=== READ %u at 0x%lx ===\n\r", read_16, (uint32_t)0x04);
        flash.write(0x00, (uint32_t)(64));
        printf("====== REWROTE 64 at 0x00 ======\n\r"); 
        flash.flush();
        read_32 = flash.read<uint32_t>(0x00);
        printf("=== READ %lu at 0x%lx ===\n\r", read_32, (uint32_t)0x00);
        flash.write(0x00, (uint32_t)(128));
        flash.flush();
        printf("====== REWROTE 128 at 0x00 ======\n\r");
        read_32 = flash.read<uint32_t>(0x00);
        printf("=== READ (again) %lu at 0x%lx ===\n\r", read_32, (uint32_t)0x00);

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