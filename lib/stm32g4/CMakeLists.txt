cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms
    USE_HAL_DRIVER
    STM32G431xx
    $<$<CONFIG:Debug>:DEBUG>
)

if(NOT DEFINED MX_SRC_DIR)
    MESSAGE(FATAL_ERROR "ERROR: Cube MX Source Directory Must Be Defined")
endif()

if(NOT DEFINED MX_INC_DIR)
    MESSAGE(FATAL_ERROR "ERROR: Cube MX Source Directory Must Be Defined")
endif()

if(NOT DEFINED MX_STARTUP_S)
    MESSAGE(FATAL_ERROR "ERROR: Cube MX Source Directory Must Be Defined")
endif()

# Point the repository-root compile database at this project.
get_filename_component(ESW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
file(MAKE_DIRECTORY ${ESW_ROOT}/build)
file(CREATE_LINK
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${ESW_ROOT}/build/compile_commands.json
    SYMBOLIC
)

# STM32CubeMX generated & fwlib include paths
set(MX_Include_Dirs
    ${MX_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/CMSIS/Device/ST/STM32G4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../rtos/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../stm32/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../util/Inc
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

# Drivers Middlewares
set(FREERTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Middlewares/Third_Party/FreeRTOS/Source)

if(MROVER_USE_RTOS)
    if(NOT EXISTS ${FREERTOS_DIR}/tasks.c)
        message(FATAL_ERROR
            "MROVER_USE_RTOS is ON but FreeRTOS was not found at ${FREERTOS_DIR}.\n"
            "Run: git submodule update --init --recursive")
    endif()

    set(FREERTOS_SOURCES
        ${FREERTOS_DIR}/croutine.c
        ${FREERTOS_DIR}/event_groups.c
        ${FREERTOS_DIR}/list.c
        ${FREERTOS_DIR}/queue.c
        ${FREERTOS_DIR}/stream_buffer.c
        ${FREERTOS_DIR}/tasks.c
        ${FREERTOS_DIR}/timers.c
        ${FREERTOS_DIR}/CMSIS_RTOS_V2/cmsis_os2.c
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
        # No heap_*.c, FreeRTOSConfig.h sets configSUPPORT_DYNAMIC_ALLOCATION 0.
    )

    list(APPEND MX_Include_Dirs
        ${FREERTOS_DIR}/include
        ${FREERTOS_DIR}/CMSIS_RTOS_V2
        ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    )
    list(APPEND MX_Defines_Syms MROVER_USE_RTOS)
endif()

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)
if(MROVER_USE_RTOS)
    list(APPEND MX_LINK_LIBS freertos fwrtos)
endif()

# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)

# Create the FreeRTOS kernel object library
if(MROVER_USE_RTOS)
    add_library(freertos OBJECT)
    target_sources(freertos PRIVATE ${FREERTOS_SOURCES})
    target_link_libraries(freertos PUBLIC stm32cubemx)

    set_source_files_properties(${MX_SRC_DIR}/stm32g4xx_it.c
        TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        PROPERTIES COMPILE_DEFINITIONS
            "SVC_Handler=mx_unused_SVC_Handler;PendSV_Handler=mx_unused_PendSV_Handler;SysTick_Handler=mx_unused_SysTick_Handler"
    )
endif()


# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()

