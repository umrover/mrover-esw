cmake_minimum_required(VERSION 3.22)

# Setup compiler settings

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Set the project name
set(CMAKE_PROJECT_NAME canalyzer)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME} LANGUAGES CXX)


add_executable(canalyzer
    Core/Src/canalyzer.cpp
    Core/Src/logger.cpp
)

target_include_directories(canalyzer PRIVATE
    Core/Inc
)

# Add lib dependency
add_subdirectory(../../lib fwlib)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}

    # Add user defined libraries
    yaml
    influxdb
    dbc_runtime
)