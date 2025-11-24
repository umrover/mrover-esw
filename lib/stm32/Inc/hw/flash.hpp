#include "main.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <array>

// suggestion flash interface based on the mjbots library
// https://github.com/mjbots/mjlib/blob/master/mjlib/micro/flash.h
namespace mrover {

  constexpr uint32_t LAST_PAGE_START = 0x0801F800;
  constexpr uint32_t LAST_PAGE_END = 0x0801FFFF;
  constexpr uint32_t PAGE_SIZE = 2048;
  constexpr int CHUNK_SIZE = 0x08;


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
    ~Flash() = default;
    Flash (uint32_t start_addr) {
      info.start = start_addr;
      info.end = LAST_PAGE_END;
    };
   
    struct Info {
      uint32_t start = 0;
      uint32_t end = 0;
    };

    // LAST PAGE OF FLASH ONLY
    Info info {
      info.start = 0x0801F800,
      info.end = 0x0801FFFF
    };

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
        printf("WRITE FAILED\n\r");
      } else {
        printf("WRITE SUCCESS\n\r");
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
    auto read_byte(uint32_t start_addr) -> uint8_t {
      uint8_t byte = *(__IO uint8_t *)start_addr;
      return byte;
    }

    // Calculate what page of flash an address is in.
    auto GetPage(uint32_t address) -> uint32_t  {
      return (address - 0x08000000) / PAGE_SIZE;
    }

    // Calculate start address of page page_num
    auto get_page_start(uint32_t page_num) -> uint32_t {
      return 0x08000000 + (page_num * PAGE_SIZE);
    }

    // get physical address from custom address
    auto get_physical_addr(uint32_t address) -> uint32_t {
      return info.start + address;
    }
    
    // get start address of double word custom address is located in
    auto get_double_word_start(uint32_t address) -> uint32_t {
      uint32_t physical_addr = get_physical_addr(address);
      return physical_addr = physical_addr & ~0x7ULL;
    }

    // writes a value in the correct double word, given its size
    template <typename T>
    void write(uint32_t address, size_t size_of, T value) {  
      uint32_t dbl_word_start = get_double_word_start(address);
      int address_offset = address % 8;

      uint64_t existing = read_double_word(dbl_word_start);

      uint64_t mask = 0;
      for (size_t i = 0; i < size_of; ++i) {
        mask |= 0xFFULL << ((address_offset + i) * 8);
      }
      
      uint64_t cleared = existing & ~mask;
      uint64_t new_data = 0;
      memcpy(&new_data, &value, size_of);
      new_data <<= (address_offset * 8);

      new_data |= cleared;
      program_double_word(dbl_word_start, new_data);
    }

    template <typename T>
    T read(uint32_t address) {
      uint32_t dbl_word_start = get_double_word_start(address);
      int address_offset = address % 8;
      uint32_t size_of = sizeof(T);
      
      uint64_t dbl_word = read_double_word(dbl_word_start);

      uint64_t shifted = dbl_word >> (address_offset * 8);

      uint64_t mask = 0;
      for (size_t i = 0; i < size_of; ++i) {
        mask |= 0xFF << (i * 8);
      }
      shifted &= mask;

      T result;
      memcpy(&result, &shifted, size_of);
      return result;

    }
  
    // copy a 2KB page of flash into array
    void copy_page(std::array<uint8_t, PAGE_SIZE> &buffer, uint32_t page_num) {
      uint32_t curr_addr = get_page_start(page_num);
      for (int i = 0; i < PAGE_SIZE; ++i) {
        uint8_t byte = read_byte(curr_addr);
        buffer[i] = byte;
      }
    }

    // once changes are finalized, erase and rewrite a flash page
    void flush(std::array<uint8_t, PAGE_SIZE> &buffer, uint32_t page_num) {
      uint32_t curr_addr = get_page_start(page_num);
      erase();
      for (int i = 0; i < PAGE_SIZE / 8; ++i) {
        uint64_t double_word = 0;
        uint32_t first = i * 8;
        for (int i = 0; i < 8; ++i) {
          double_word |= (buffer[first + i] << (8 * i));
        }
        program_double_word(curr_addr, double_word);
        curr_addr += 8;
      }
    }
  
  };

}