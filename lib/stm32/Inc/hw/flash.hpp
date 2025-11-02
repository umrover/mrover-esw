#include "main.h"

// suggestion flash interface based on the mjbots library
// https://github.com/mjbots/mjlib/blob/master/mjlib/micro/flash.h
namespace mrover {

  class Flash {
  public:
    Flash() = default;
    ~Flash() = default;
   
    struct Info {
      char* start = nullptr;
      char* end = nullptr;
    };
    Info info;

    /// Get information necessary to read or write the flash.  Reading
    /// may be accomplished by direct memory access.  Writing must use
    /// the ProgramByte method below following an Erase.
    /// LAST PAGE ONLY
    void get_info() {
      info.start = (char*)0x0801F800;
      info.end = (char*)0x0801FFFF; // inclusive
    }

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
        
      }
      
    }

    /// Unlock/lock the flash protection
    void unlock() {
      HAL_FLASH_Unlock();
    }
    void lock() { 
      HAL_FLASH_Lock();
    }

    /// Write a double word into flash.
    void program_word(uint32_t address, uint64_t value) {
      unlock();
      erase();
      HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, value);
      lock();

      if (status != HAL_OK) {
        
      }
    }

    uint64_t read_word(uint32_t start_addr, int num_words) {
      uint64_t word = *(__IO uint64_t *)start_addr;
      return word;
    } 

    int GetPage(uint32_t address) {
      return (address - 0x08000000) / 2048;
    }
  };

}