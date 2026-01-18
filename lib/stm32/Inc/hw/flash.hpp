#include "main.h"
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

    // --------------------------------------------------------------------
    // FLASH CLASS
    // --------------------------------------------------------------------
    template<typename T>
    concept mem_layout_t = requires {
        { T::FLASH_BEGIN_ADDR } -> std::convertible_to<uint32_t>;
        { T::FLASH_END_ADDR } -> std::convertible_to<uint32_t>;
        { T::PAGE_SIZE } -> std::convertible_to<int>;
        { T::NUM_PAGES } -> std::convertible_to<int>;
    };

    /*
  constexpr uint32_t LAST_PAGE_START = 0x0801F800;
  constexpr uint32_t LAST_PAGE_END = 0x0801FFFF;
  constexpr uint32_t PAGE_SIZE = 2048;
  constexpr uint32_t NUM_PAGES = 64;
  */

    template<config_t Config, mem_layout_t Mem>
    class Flash {
    private:
        std::array<uint8_t, Mem::PAGE_SIZE> m_page_buffer;
        uint32_t m_loaded_page_num;
        bool m_dirty;
        static constexpr uint32_t m_flash_end = Mem::FLASH_END_ADDR;
        static constexpr uint32_t m_flash_begin = Mem::FLASH_BEGIN_ADDR;
        static constexpr int m_page_size = Mem::PAGE_SIZE;
        static constexpr int m_num_pages = Mem::NUM_PAGES;
        uint32_t m_last_page_start = m_flash_begin + m_page_size * (m_num_pages - 1);

        // Unlock/lock the flash protection
        void unlock() {
            HAL_FLASH_Unlock();
        }
        void lock() {
            HAL_FLASH_Lock();
        }

        void load_page(uint32_t page_num) {
            uint32_t curr_addr = get_page_start(page_num);
            for (size_t i = 0; i < m_page_size; ++i) {
                uint8_t byte = read_byte(curr_addr + i);
                m_page_buffer[i] = byte;
            }
            m_loaded_page_num = page_num;
            m_dirty = false;
        }

        // Calculate what page of flash an address is in.
        auto get_page(uint32_t address) -> uint32_t {
            return (address - m_flash_begin) / m_page_size;
        }

        // Calculate start address of page page_num
        auto get_page_start(uint32_t page_num) -> uint32_t {
            return m_flash_begin + (page_num * m_page_size);
        }

        // get physical address from custom address
        auto get_physical_addr(uint32_t address) -> uint32_t {
            return m_region_start + address;
        }

        // get start address of double word custom address is located in
        auto get_double_word_start(uint32_t address) -> uint32_t {
            uint32_t physical_addr = get_physical_addr(address);
            return physical_addr = physical_addr & ~0x7ULL;
        }

        /// Erase the entirety of the managed flash section.
        void erase_page(uint32_t const& page_num = (m_num_pages - 1)) {
            unlock();

            // uint32_t page_number = ((uint32_t)m_region_start - 0x08000000) / 0x800;
            uint32_t start_page_number = page_num;
            uint32_t end_page_number = page_num;
            uint32_t num_pages = ((end_page_number - start_page_number) / FLASH_PAGE_SIZE) + 1;

            FLASH_EraseInitTypeDef erase_init;
            erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
            erase_init.Page = start_page_number;
            erase_init.NbPages = num_pages;

            uint32_t page_error = 0;
            HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);
            lock();

            if (status != HAL_OK) {
                // TODO logger for this
                // printf("ERASE FAILED\n\r");
            }
        }

    public:
        uint32_t m_region_start = m_last_page_start; // default to last page only
        uint32_t m_region_end = m_flash_end;

        Flash() : m_loaded_page_num(UINT32_MAX), m_dirty(false) {
            static_assert(validated_config_t<Config>::is_valid(), "consumed config is valid");
        };
        ~Flash() = default;
        Flash(uint32_t start_addr) : m_loaded_page_num(UINT32_MAX), m_dirty(false) {
            static_assert(validated_config_t<Config>::is_valid(), "consumed config is valid");
            m_region_start = start_addr;
            m_region_end = m_flash_end;
        };

        auto get_start() -> uint32_t {
            return m_region_start;
        }

        /// Write a double word into flash.
        void program_double_word(uint32_t address, uint64_t value) {
            unlock();
            // printf ("address: 0x%lx\n\r", address);
            HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, value);
            lock();

            if (status != HAL_OK) {
                // TODO logger for this
                // printf("WRITE FAILED\n\r");
            }
        }

        // Read a double word from flash.
        auto read_double_word(uint32_t start_addr) -> uint64_t {
            uint64_t word = *(__IO uint64_t*) start_addr;
            return word;
        }
        auto read_byte(uint32_t start_addr) -> uint8_t {
            uint8_t byte = *(__IO uint8_t*) start_addr;
            return byte;
        }

        template<typename T>
        void write(uint32_t const& custom_addr, T const& value) {
            uint32_t physical_addr = m_region_start + custom_addr;
            uint32_t page_num = get_page(physical_addr);
            uint32_t offset = physical_addr - get_page_start(page_num);

            if (page_num != m_loaded_page_num) {
                if (m_dirty) flush();
                load_page(page_num);
            }

            memcpy(&m_page_buffer[offset], &value, sizeof(T));
            m_dirty = true;
        }

        template<typename T>
        T read(uint32_t address) {

            uint32_t physical_addr = get_physical_addr(address);

            T result;
            memcpy(&result, (void*) physical_addr, sizeof(T));
            return result;
        }


        // once changes are finalized, erase and rewrite a flash page
        void flush() {
            if (!m_dirty || m_loaded_page_num == UINT32_MAX) return;

            uint32_t start_addr = get_page_start(m_loaded_page_num);
            erase_page(m_loaded_page_num);

            for (size_t i = 0; i < m_page_size / 8; ++i) {
                uint64_t double_word = 0;
                for (int j = 0; j < 8; ++j) {
                    double_word |= ((uint64_t) m_page_buffer[i * 8 + j]) << (8 * j);
                }
                program_double_word(start_addr + (i * 8), double_word);
            }

            m_dirty = false;
            load_page(m_loaded_page_num);
            // TODO logger for this
            // printf("====== PAGE FLUSHED ======\n\r");
        }

        template<typename Field, typename Value>
        auto write_config(Field const& f, Value const& v) {
            using field_type = typename Field::value_t;
            write(f.addr, static_cast<field_type>(v));
            flush();
        }

        template<typename Field>
        auto read_config(Field const& f) -> Field::value_t {
            using V = Field::value_t;
            V value{};
            value = read<V>(f.addr);
            return value;
        }
    };

} // namespace mrover
