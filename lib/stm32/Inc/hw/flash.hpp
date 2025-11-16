#include "main.h"
#include <cstdint>
#include <cstring>
#include <cstdio>

// suggestion flash interface based on the mjbots library
// https://github.com/mjbots/mjlib/blob/master/mjlib/micro/flash.h
namespace mrover {

  <template typename Cfg>

  class Flash {
  private:
    // Unlock/lock the flash protection
    void unlock() {
      HAL_FLASH_Unlock();
    }
    void lock() { 
      HAL_FLASH_Lock();
    }

  public:
    Flash() = default;
    Flash(const Cfg &config_in) : config(config_in) {};
    ~Flash() = default;
   
    struct Info {
      char* start = nullptr;
      char* end = nullptr;
    };

    // LAST PAGE OF FLASH ONLY
    Info info {
      info.start = (char*)0x0801F800,
      info.end = (char*)0x0801FFFF
    };
    Cfg config;

    /// Erase the entirety of the managed flash section.
    void erase() {
      unlock();

      // uint32_t page_number = ((uint32_t)info.start - 0x08000000) / 0x800;
      uint32_t start_page_number = GetPage((uint32_t)info.start);
      uint32_t end_page_number = GetPage((uint32_t)info.end);
      uint32_t num_pages = ((end_page_number - start_page_number)/FLASH_PAGE_SIZE) +1;

      FLASH_EraseInitTypeDef erase_init;
      erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
      erase_init.Page = start_page_number;
      erase_init.NbPages = num_pages;

      uint32_t page_error = 0;
      HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);
      lock();

      if (status != HAL_OK) {
        printf("ERASE FAILED");
      }
      
    }


    /// Write a double word into flash.
    void program_double_word(uint32_t address, uint64_t value) {
      unlock();
      // printf("WRITING ADDRESS 0x%lx, WRITE VALUE: %lu\n\r", address, value);
      HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, value);
      lock();

      if (status != HAL_OK) {
        printf("WRITE FAILED");
      }
    }

    // Write a float into flash
    void write_float(uint32_t address, const float &value) {
      uint32_t raw_float = 0;
      uint64_t float_64 = 0;
      memcpy(&raw_float, &value, 4);
      float_64 = (uint64_t)raw_float;
      program_double_word(address, float_64); 
    }

    auto read_float(uint32_t address) -> float {
      float read = 0.0;
      uint32_t raw_read = (uint32_t)(read_double_word(address) & 0xFFFFFFFF);

      memcpy(&read, &raw_read, 4);
      return read;
    }

    // Read a double word from flash.
    auto read_double_word(uint32_t start_addr) -> uint64_t {
      uint64_t word = *(__IO uint64_t *)start_addr;
      // printf("READING ADDRESS 0x%lx, READ VALUE: %lu\n\r", start_addr, word);
      return word;
    } 

    // Calculate what page of flash an address is in.
    auto GetPage(uint32_t address) -> uint32_t  {
      return (address - 0x08000000) / 2048;
    }

  };

}