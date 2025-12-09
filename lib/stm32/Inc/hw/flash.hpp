#include "main.h"
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string_view>
#include <tuple>


// suggestion flash interface based on the mjbots library
// https://github.com/mjbots/mjlib/blob/master/mjlib/micro/flash.h
namespace mrover {

// -------------------------------------------------------------------
// CONFIG IMPLEMENTATIONS
// -------------------------------------------------------------------


template<typename T>
concept config_t = requires {
    { T::all() } -> std::same_as<decltype(T::all())>;
    typename std::tuple_size<std::remove_cvref_t<decltype(T::all())>>::type;
    { T::size_bytes() } -> std::same_as<uint16_t>;
};

template<typename T>
struct reg_t {
    using value_t = T;

    std::string_view name;
    uint16_t addr{};
    static consteval size_t size() { return sizeof(T); }
    [[nodiscard]] constexpr uint16_t reg() const { return addr; }
};

template<config_t Config>
struct config_validator_t {
private:
    static constexpr auto tup = Config::all();
    static constexpr size_t N = std::tuple_size_v<decltype(tup)>;

    struct reg_descriptor_t {
        std::string_view name;
        uint16_t addr{};
        size_t size{};
        [[nodiscard]] constexpr uint16_t reg() const { return addr; }
    };

    template<std::size_t... Is>
    static constexpr auto tuple_to_array(std::index_sequence<Is...>) {
        return std::array<reg_descriptor_t, N>{
                reg_descriptor_t{
                        std::get<Is>(tup).name,
                        std::get<Is>(tup).addr,
                        std::get<Is>(tup).size()}...};
    }

    static constexpr auto unsorted =
            tuple_to_array(std::make_index_sequence<N>{});

    static constexpr auto fields = [] {
        auto tmp = unsorted;
        std::sort(tmp.begin(), tmp.end(),
                  [](auto const& a, auto const& b) { return a.reg() < b.reg(); });
        return tmp;
    }();

public:
    static consteval bool is_valid() {
        if (N == 0) return false;
        for (size_t i = 1; i < N; ++i) {
            auto const& prev = fields[i - 1];
            auto const& curr = fields[i];
            if (curr.addr < prev.addr + prev.size) return false;
        }
        return true;
    }

    static consteval uint16_t size_bytes() {
        if constexpr (N == 0) return 0;
        auto constexpr last_field = fields[N - 1];
        return last_field.addr + last_field.size;
    }
};

template<typename Config>
struct validated_config_t {
    static consteval bool is_valid() {
        return config_validator_t<Config>::is_valid();
    }

    static consteval uint16_t size_bytes() {
        return config_validator_t<Config>::size_bytes();
    }
};

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

// --------------------------------------------------------------------
// FLASH CLASS
// --------------------------------------------------------------------



  constexpr uint32_t LAST_PAGE_START = 0x0801F800;
  constexpr uint32_t LAST_PAGE_END = 0x0801FFFF;
  constexpr uint32_t PAGE_SIZE = 2048;
  constexpr int CHUNK_SIZE = 0x08;
  constexpr uint32_t LAST_PAGE_NUM = 63;

  template <config_t Config>
  class Flash {
  private:
    // Unlock/lock the flash protection
    void unlock() {
      HAL_FLASH_Unlock();
    }
    void lock() { 
      HAL_FLASH_Lock();
    }

    std::array<uint8_t, PAGE_SIZE> page_buffer;
    uint32_t loaded_page_num;
    bool dirty;

    void load_page( uint32_t page_num) {
      uint32_t curr_addr = get_page_start(page_num);
      for (size_t i = 0; i < PAGE_SIZE; ++i) {
        uint8_t byte = read_byte(curr_addr + i);
        page_buffer[i] = byte;
      }
      loaded_page_num = page_num;
      dirty = false;
    }

    // Calculate what page of flash an address is in.
    auto get_page(uint32_t address) -> uint32_t  {
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

  public:
    Flash() {
      static_assert(validated_config_t<Config>::is_valid(), "consumed config is valid");
    };
    ~Flash() = default;
    Flash (uint32_t start_addr) {
      static_assert(validated_config_t<Config>::is_valid(), "consumed config is valid");
      region_start = start_addr;
      region_end = LAST_PAGE_END;
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

    uint32_t region_start = LAST_PAGE_START;
    uint32_t region_end = LAST_PAGE_END;

    auto get_start() -> uint32_t {
      return info.start;
    }

    /// Erase the entirety of the managed flash section.
    void erase_page(const uint32_t &page_num = LAST_PAGE_NUM) {
      unlock();

      // uint32_t page_number = ((uint32_t)info.start - 0x08000000) / 0x800;
      uint32_t start_page_number = page_num;
      uint32_t end_page_number = page_num;
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
      }
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

    template <typename T>
    void write(const uint32_t &custom_addr, T value) {
      uint32_t physical_addr = region_start + custom_addr;
      uint32_t page_num = get_page(physical_addr);
      uint32_t offset = physical_addr - get_page_start(page_num);

      if (page_num != loaded_page_num) {
        if (dirty) flush();
        load_page(page_num);
      }

      printf("====== WROTE AT ADDRESS: 0x%lu, PHYSICAL ADDRESS: 0x%lx ======\n\r", 
        custom_addr, physical_addr);
      memcpy(&page_buffer[offset], &value, sizeof(T));
      dirty = true;

    }

    template <typename T>
    T read(uint32_t address) {
      
      uint32_t physical_addr = get_physical_addr(address);

      T result;
      memcpy(&result, (void *)physical_addr, sizeof(T));
      return result;

    }
  

    // once changes are finalized, erase and rewrite a flash page
    void flush() {
      if (!dirty || loaded_page_num == UINT32_MAX) return;
      
      uint32_t start_addr = get_page_start(loaded_page_num);
      erase_page(loaded_page_num);

      for (size_t i = 0; i < PAGE_SIZE / 8; ++i) {
        uint64_t double_word = 0;
        for (int j = 0; j < 8; ++j) {
          double_word |= ((uint64_t)page_buffer[i * 8 + j]) << (8 * j);
        }
        program_double_word(start_addr + (i * 8), double_word);
        
      }

      dirty = false;
      printf("====== PAGE FLUSHED ======\n\r");
    }

    template<typename Field, typename Value>
    auto write_config(Field const& f, Value const& v) {
      //std::cout << "Writing value " << v << " to register " << f.name << " at 0x" << std::hex << f.addr << std::dec << "\n";
      write(f.addr, v);
      flush();
    }

    template<typename Field>
    auto read_config(Field const& f) -> Field::value_t {
      using V = Field::value_t;
      V value{};
      value = read<V>(f.addr);
      //std::cout << "Read value " << value << " from register " << f.name << " at 0x" << std::hex << f.addr << std::hex << "\n";
      return value;
    }
  
  };

}