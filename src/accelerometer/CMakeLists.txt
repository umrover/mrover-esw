cmake_minimum_required(VERSION 3.22)

# Setup compiler settings

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Set the project name
set(CMAKE_PROJECT_NAME accelerometer)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME} LANGUAGES CXX)


add_executable(accelerometer
    Core/Src/accelerometer.cpp
    Core/Src/i2c_bus.cpp
    Core/Src/mpu6050.cpp
    Core/Src/committer.cpp
)

target_include_directories(accelerometer PRIVATE
    Core/Inc
)

target_compile_options(accelerometer PRIVATE -Wall -Wextra)

# Add lib dependency
add_subdirectory(../../lib fwlib)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}

    # Add user defined libraries
    yaml
    influxdb
)
