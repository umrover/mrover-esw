#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

GENERATE_SCRIPT="$TOOLS_DIR/scripts/generate_project.py"
CMAKE_SCRIPT="$TOOLS_DIR/scripts/update_cmake_cfg.py"

MCU=""
BOARD=""
SRC=""
LIBS=()
RTOS=false
CCMSRAM_ARGS=()

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME [--mcu <mcu> | --board <board>] --src <source> [--lib <library>]+ [--rtos] [--ccmsram]

Create a new CubeMX firmware project.

options:
  -m, --mcu <mcu>       target mcu for new project
  -b, --board <board>   target board for new project
  -s, --src <source>    path to project root
  -l, --lib <library>   cmake library dependency
  -r, --rtos            build with FreeRTOS (CMSIS-RTOS v2)
  -c, --ccmsram         place marked functions in CCM SRAM
  -h, --help            show this help message
EOF
    exit "${1:-0}"
}

# a library is a lib/ directory that defines a cmake library of the same name
valid_libs() {
    local dir
    for dir in "$ESW_ROOT"/lib/*(/); do
        grep -qE "add_library\(${dir:t}([[:space:]]|\)|$)" "$dir/CMakeLists.txt" 2> /dev/null && print -r -- "${dir:t}"
    done
    return 0
}

# parse opts
while [[ $# -gt 0 ]]; do
    case "$1" in
        -m|--mcu)       need_value "$1" $#; MCU="$2"; shift 2 ;;
        -b|--board)     need_value "$1" $#; BOARD="$2"; shift 2 ;;
        -s|--src)       need_value "$1" $#; SRC="${2:A}"; shift 2 ;;
        -l|--lib)       need_value "$1" $#; LIBS+=("$2"); shift 2 ;;
        -r|--rtos)      RTOS=true; shift ;;
        -c|--ccmsram)   CCMSRAM_ARGS=(--ccmsram); shift ;;
        -h|--help)      usage ;;
        *)              unknown_option "$1" ;;
    esac
done

# select project source
[[ -n "$SRC" ]] || die "project source path (\`--src\`) must be specified"

# assert target directory does not exist
[[ ! -d "$SRC" ]] || die "project source path must not exist already"

# select target
if { [[ -n "$MCU" ]] && [[ -n "$BOARD" ]]; } || { [[ -z "$MCU" ]] && [[ -z "$BOARD" ]]; }; then
    die "exactly one of \`--mcu\` or \`--board\` must be specified"
fi
TARGET="${MCU:-$BOARD}"

# reject unknown libraries before cubemx generates anything
VALID_LIBS=(${(f)"$(valid_libs)"})
for lib in "${LIBS[@]}"; do
    if ((! ${VALID_LIBS[(Ie)$lib]})); then
        [[ "$lib" == "rtos" ]] && die "\`rtos\` is not a \`--lib\`; use \`--rtos\`"
        valid=""
        for name in "${VALID_LIBS[@]}"; do valid+="${valid:+, }\`$name\`"; done
        die "unknown library \`$lib\` (valid: $valid)"
    fi
done

require uv

# run project generation script (uv run creates and syncs tools/.venv on demand)
esw_uv python "$GENERATE_SCRIPT" --mcu "$TARGET" --src "$SRC"

# create args
PY_LIB_ARGS=()
for lib in "${LIBS[@]}"; do
    PY_LIB_ARGS+=(--lib "$lib")
done

# RTOS is not managed by CubeMX
PY_RTOS_ARGS=()
if [[ "$RTOS" == "true" ]]; then
    PY_RTOS_ARGS+=(--rtos)
fi

# run cmake configuration script
esw_uv python "$CMAKE_SCRIPT" \
    --src "$SRC" \
    --root "$ESW_ROOT" \
    --ctx "$ESW_ROOT/lib/stm32g4" \
    "${PY_LIB_ARGS[@]}" \
    "${PY_RTOS_ARGS[@]}" \
    "${CCMSRAM_ARGS[@]}"
