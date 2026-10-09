cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
IF(NOT DEFINED MX_SRC_DIR)
    MESSAGE(FATAL_ERROR "ERROR: Cube MX Source Directory Must Be Defined")
ENDIF()

IF(NOT DEFINED MX_INC_DIR)
    MESSAGE(FATAL_ERROR "ERROR: Cube MX Source Directory Must Be Defined")
ENDIF()

IF(NOT DEFINED MX_STARTUP_S)
    MESSAGE(FATAL_ERROR "ERROR: Cube MX Source Directory Must Be Defined")
ENDIF()

# Device define from the CubeMX startup file
get_filename_component(MX_STARTUP_NAME ${MX_STARTUP_S} NAME_WE)
string(REGEX REPLACE "^startup_stm32(g4[0-9]+)xx$" "\\1" MX_PART ${MX_STARTUP_NAME})
if(MX_PART STREQUAL MX_STARTUP_NAME)
    message(FATAL_ERROR "Cannot derive STM32G4 device from ${MX_STARTUP_NAME}")
endif()
string(TOUPPER ${MX_PART} MX_PART)

# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms
    USE_HAL_DRIVER
    STM32${MX_PART}xx
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${MX_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/CMSIS/Include
)

# Point the repository-root compile database at active project
get_filename_component(ESW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
file(MAKE_DIRECTORY ${ESW_ROOT}/build)
file(CREATE_LINK
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${ESW_ROOT}/build/compile_commands.json
    SYMBOLIC
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${MX_SRC_DIR}/main.c
    ${MX_SRC_DIR}/stm32g4xx_it.c
    ${MX_SRC_DIR}/stm32g4xx_hal_msp.c
    ${MX_SRC_DIR}/sysmem.c
    ${MX_SRC_DIR}/syscalls.c
    ${MX_STARTUP_S}
)

# STM32 HAL/LL Drivers
file(GLOB_RECURSE DRIVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Src/*.c"
)
list(FILTER DRIVER_SOURCES EXCLUDE REGEX ".*_template\\.c$")
set(STM32_Drivers_Src
    ${MX_SRC_DIR}/system_stm32g4xx.c
    ${DRIVER_SOURCES}
)

# Drivers Midllewares



# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)

# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Optional CCM SRAM code section (`CCMSRAM` functions)
if(MX_USE_CCMSRAM)
    # CCM SRAM size/alias address per device family
    if(MX_PART MATCHES "^G4[78]") # G471/G473/G474/G483/G484
        set(MX_CCMSRAM_SIZE 32K)
        set(MX_CCMSRAM_ALIAS 0x20018000)
    elseif(MX_PART MATCHES "^G4[9A]") # G491/G4A1
        set(MX_CCMSRAM_SIZE 16K)
        set(MX_CCMSRAM_ALIAS 0x20018000)
    elseif(MX_PART MATCHES "^G4[34]") # G431/G441
        set(MX_CCMSRAM_SIZE 10K)
        set(MX_CCMSRAM_ALIAS 0x20005800)
    else()
        message(FATAL_ERROR "No CCM SRAM layout for STM32${MX_PART}xx")
    endif()
    set(MX_CCMSRAM_LD "${CMAKE_BINARY_DIR}/ccmsram.ld")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ccmsram.ld.in" "${MX_CCMSRAM_LD}" @ONLY)
    target_link_libraries(${CMAKE_PROJECT_NAME} "-T\"${MX_CCMSRAM_LD}\"")
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${MX_CCMSRAM_LD}")
endif()

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()

