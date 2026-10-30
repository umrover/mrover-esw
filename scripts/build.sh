#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

VENV_PATH="$TOOLS_DIR/.venv"
SRC=""
PRESET="Debug"
TARGET_NAME=""
DO_FLASH=false
DO_CLEAN=false
DO_MAP=false

STLINK_PORT="${STLINK_PORT:-swd}"
STLINK_FREQ="${STLINK_FREQ:-8000}"
STLINK_RESET="${STLINK_RESET:-HWrst}"

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME --src <src> [options]

Build a firmware project with the ESW toolchain.

options:
  -s, --src <path>      relative path to source directory (required)
  -p, --preset <name>   cmake preset (default: Debug)
  -t, --target <name>   cmake target name (default: folder name)
  -f, --flash           flash the device after build
  -m, --map             print a flash size breakdown after build
  -c, --clean           clean build directory
  -h, --help            show this help message
EOF
    exit "${1:-0}"
}

run_step() {
    local desc="$1"
    shift
    info "step: ${desc}"
    if "$@"; then
        ok "success: ${desc}"
    else
        printf "%b\n" "${RED}✗ failed: ${desc}${NC}"
        exit 1
    fi
}

clean() {
    local build_dir="$1"
    local venv_dir="$2"
    local dbc_dir="$3"
    rm -rf "$build_dir"
    rm -rf "$venv_dir"
    rm -rf "$dbc_dir"
}

# parse flags
while [[ $# -gt 0 ]]; do
    case "$1" in
        -s|--src)       need_value "$1" $#; SRC="${2:A}"; shift 2 ;;
        -p|--preset)    need_value "$1" $#; PRESET="$2"; shift 2 ;;
        -t|--target)    need_value "$1" $#; TARGET_NAME="$2"; shift 2 ;;
        -f|--flash)     DO_FLASH=true; shift ;;
        -m|--map)       DO_MAP=true; shift ;;
        -c|--clean)     DO_CLEAN=true; shift ;;
        -h|--help)      usage ;;
        *)              unknown_option "$1" ;;
    esac
done

if [[ -z "$SRC" ]]; then
    usage_error "\`-s/--src\` required"
fi

SRC_DIR="$SRC"
[[ -d "$SRC_DIR" ]] || die "directory $SRC_DIR does not exist"

if [[ -z "$TARGET_NAME" ]]; then
    TARGET_NAME="${SRC:t}"
fi

require cmake ninja uv
if [[ "$DO_FLASH" == "true" ]]; then
    require STM32_Programmer_CLI
fi

SUBMODULE_DIR="$ESW_ROOT/lib/stm32g4/STM32CubeG4"
[[ -e "$SUBMODULE_DIR/.git" ]] \
    || die "submodule not found at $SUBMODULE_DIR (run \`git submodule update --init --recursive\`)"

info "====== project: ${YELLOW}$TARGET_NAME${BLUE} | preset: ${YELLOW}$PRESET${BLUE} ======"

pushd "$SRC_DIR" > /dev/null
BUILD_DIR="build/$PRESET"

# clean the project if that paramter was set
if [[ "$DO_CLEAN" == "true" ]]; then
    run_step "cleaning build dir" clean "$BUILD_DIR" "$VENV_PATH" "$ESW_ROOT/lib/dbc/build"
    exit 0
fi

# configure cmake
mkdir -p "$BUILD_DIR"
run_step "configure cmake" cmake --preset "$PRESET"

# execute build
run_step "build target" cmake --build --target "$TARGET_NAME" --preset "$PRESET"

# print the flash size breakdown
if [[ "$DO_MAP" == "true" ]]; then
    LD_SCRIPT=$(find "$SRC_DIR" -maxdepth 1 -name '*.ld' | head -n 1)
    run_step "flash breakdown" esw_uv python "$TOOLS_DIR/scripts/flash_usage.py" \
        --elf "$BUILD_DIR/${TARGET_NAME}.elf" \
        --ld "$LD_SCRIPT" \
        --inc "$SRC_DIR/Inc" \
        --map "$BUILD_DIR/${TARGET_NAME}.map" \
        --report
fi

# flash if parameter set
if [[ "$DO_FLASH" == "true" ]]; then
    ELF="$BUILD_DIR/${TARGET_NAME}.elf"
    [[ -f "$ELF" ]] || die "flash failed: could not find elf file at $ELF"

    FLASH_CMD=(STM32_Programmer_CLI --connect port="$STLINK_PORT" freq="$STLINK_FREQ" reset="$STLINK_RESET")
    FLASH_CMD+=(--write "$ELF")
    FLASH_CMD+=(--verify)
    FLASH_CMD+=(--start)

    run_step "flash mcu" "${FLASH_CMD[@]}"
fi

popd > /dev/null

printf "%b\n" "${GREEN}====== success ======${NC}"
