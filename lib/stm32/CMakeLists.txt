cmake_minimum_required(VERSION 3.22)

file(GLOB STM32_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

if(STM32_SOURCES)
    add_library(stm32 STATIC ${STM32_SOURCES})
    set(stm32_scope PUBLIC)
else()
    add_library(stm32 INTERFACE)
    set(stm32_scope INTERFACE)
endif()

target_include_directories(stm32 INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)

target_link_libraries(stm32 ${stm32_scope}
    util
)
