#!/usr/bin/env bash

set -euo pipefail

RED="\e[1;31m"
GREEN="\e[1;32m"
BLUE="\e[1;34m"
YELLOW="\e[1;33m"
NC="\e[0m"

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"
TOOLS_DIR="$ESW_ROOT/tools"
VENV_PATH="$TOOLS_DIR/venv"
SRC=""
PRESET="Debug"
TARGET_NAME=""
DO_FLASH=false
DO_CLEAN=false
DO_MAP=false

PORT="${PORT:-swd}"
FREQ="${FREQ:-8000}"
RESET="${RESET:-HWrst}"
SCRIPT_NAME=$(basename "$0")

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME --src <src> [options]

options:
  -s, --src <path>      relative path to source directory (required)
  -p, --preset <name>   cmake preset (default: Debug)
  -t, --target <name>   cmake target name (default: folder name)
  -f, --flash           flash the device after build
  -m, --map             print a flash size breakdown after build
  -c, --clean           clean build directory
  -h, --help            show this help message
EOF
    exit 1
}

run_step() {
    local desc="$1"
    shift
    printf "%b\n" "${BLUE}step: ${desc}${NC}"
    if "$@"; then
        printf "%b\n" "${GREEN}✓ success: ${desc}${NC}"
    else
        printf "%b\n" "${RED}✗ failed: ${desc}${NC}"
        exit 1
    fi
}

check_deps() {
    local deps=("cmake" "ninja")
    [[ "$DO_FLASH" == "true" ]] && deps+=("STM32_Programmer_CLI")
    for cmd in "${deps[@]}"; do
        if ! command -v "$cmd" &> /dev/null; then
            printf "%b\n" "${RED}✗ error: command '$cmd' not found in PATH${NC}"
            exit 1
        fi
    done
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
        -s|--src)       SRC="$(realpath "$2")"; shift 2 ;;
        -p|--preset)    PRESET="$2"; shift 2 ;;
        -t|--target)    TARGET_NAME="$2"; shift 2 ;;
        -f|--flash)     DO_FLASH=true; shift ;;
        -m|--map)       DO_MAP=true; shift ;;
        -c|--clean)     DO_CLEAN=true; shift ;;
        -h|--help)      usage ;;
        *)              printf "%b\n" "${RED}✗ unknown option: $1${NC}"; usage ;;
    esac
done

if [[ -z "$SRC" ]]; then
    printf "%b\n" "${RED}✗ error: -s/--src required${NC}"
    usage
fi

SRC_DIR="$SRC"
if [ ! -d "$SRC_DIR" ]; then
    printf "%b\n" "${RED}✗ failed: directory $SRC_DIR does not exist${NC}"
    exit 1
fi

if [[ -z "$TARGET_NAME" ]]; then
    TARGET_NAME=$(basename "$SRC")
fi

check_deps

printf "%b\n" "${BLUE}====== project: ${YELLOW}$TARGET_NAME${BLUE} | preset: ${YELLOW}$PRESET${BLUE} ======${NC}"

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
    run_step "flash breakdown" "$VENV_PATH/bin/python" "$TOOLS_DIR/scripts/flash_usage.py" \
        --elf "$BUILD_DIR/${TARGET_NAME}.elf" \
        --ld "$LD_SCRIPT" \
        --inc "$SRC_DIR/Inc" \
        --map "$BUILD_DIR/${TARGET_NAME}.map" \
        --report
fi

# flash if parameter set
if [[ "$DO_FLASH" == "true" ]]; then
    ELF="$BUILD_DIR/${TARGET_NAME}.elf"
    if [[ ! -f "$ELF" ]]; then
        printf "%b\n" "${RED}✗ flash failed: Could not find elf file at $ELF${NC}"
        exit 1
    fi

    FLASH_CMD=(STM32_Programmer_CLI --connect port="$PORT" freq="$FREQ" reset="$RESET")
    FLASH_CMD+=(--write "$ELF")
    FLASH_CMD+=(--verify)
    FLASH_CMD+=(--start)

    run_step "flash mcu" "${FLASH_CMD[@]}"
fi

popd > /dev/null

printf "%b\n" "${GREEN}====== success ======${NC}"
