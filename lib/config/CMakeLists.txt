cmake_minimum_required(VERSION 3.22)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

set(CONFIG_DIR "${PROJECT_ROOT}/config")
set(CONFIG_GEN_DIR "${MX_INC_DIR}")
set(CONFIG_TEMPLATE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set(CONFIG_TEMPLATE "${CONFIG_TEMPLATE_DIR}/config_header.hpp.j2")

file(MAKE_DIRECTORY ${CONFIG_GEN_DIR})

set(CONFIG_SCRIPT "${TOOLS_DIR}/scripts/config_gen.py")
if (NOT EXISTS ${CONFIG_SCRIPT})
    message(FATAL_ERROR "could not find script ${CONFIG_SCRIPT}")
endif()

set(CONFIG_YAML "${CONFIG_DIR}/${CMAKE_PROJECT_NAME}.yaml")
set(CONFIG_OUTPUT "${CONFIG_GEN_DIR}/${CMAKE_PROJECT_NAME}_config.hpp")

if(NOT EXISTS ${CONFIG_YAML})
    message(WARNING "could not find config yaml ${CONFIG_YAML}")
    return()
endif()

add_custom_command(
    OUTPUT ${CONFIG_OUTPUT}
    COMMAND ${VENV_PYTHON} ${CONFIG_SCRIPT}
        --name ${CMAKE_PROJECT_NAME}
        --input  ${CONFIG_YAML}
        --output ${CONFIG_OUTPUT}
        --template-dir ${CONFIG_TEMPLATE_DIR}
    DEPENDS ${CONFIG_YAML} ${CONFIG_SCRIPT} ${CONFIG_TEMPLATE} python_env_ready
    WORKING_DIRECTORY ${TOOLS_DIR}
    COMMENT "generating config header for ${CMAKE_PROJECT_NAME}"
    VERBATIM
)

if(EXISTS ${CONFIG_YAML})
    add_library(config INTERFACE ${CONFIG_OUTPUT})
    target_include_directories(config INTERFACE ${CONFIG_GEN_DIR})
endif()
