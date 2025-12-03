#!/usr/bin/env zsh

set -euo pipefail

RED="\e[1;31m"
GREEN="\e[1;32m"
BLUE="\e[1;34m"
YELLOW="\e[1;33m"
NC="\e[0m"

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"
SRC=""
PRESET="Debug"
TARGET_NAME=""
DO_FLASH=false
DO_CLEAN=false

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
  -c, --clean           clean build directory before building
  -h, --help            show this help message
EOF
    exit 1
}

run_step() {
    local desc="$1"
    shift
    echo -e "${BLUE}step: ${desc}${NC}"
    if "$@"; then
        echo -e "${GREEN}✓ success: ${desc}${NC}"
    else
        echo -e "${RED}✗ failed: ${desc}${NC}"
        exit 1
    fi
}

check_deps() {
    local deps=("cmake" "ninja")
    [[ "$DO_FLASH" == "true" ]] && deps+=("STM32_Programmer_CLI")
    for cmd in "${deps[@]}"; do
        if ! command -v "$cmd" &> /dev/null; then
            echo -e "${RED}✗ error: command '$cmd' not found in PATH${NC}"
            exit 1
        fi
    done
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -s|--src)       SRC="$2"; shift 2 ;;
        -p|--preset)    PRESET="$2"; shift 2 ;;
        -t|--target)    TARGET_NAME="$2"; shift 2 ;;
        -f|--flash)     DO_FLASH=true; shift ;;
        -c|--clean)     DO_CLEAN=true; shift ;;
        -h|--help)      usage ;;
        *)              echo -e "${RED}✗ unknown option: $1${NC}"; usage ;;
    esac
done

if [[ -z "$SRC" ]]; then
    echo -e "${RED}✗ error: -s/--src required${NC}"
    usage
fi

SRC_DIR="$ESW_ROOT/$SRC"
if [ ! -d "$SRC_DIR" ]; then
    echo -e "${RED}✗ failed: directory $SRC_DIR does not exist${NC}"
    exit 1
fi

if [[ -z "$TARGET_NAME" ]]; then
    TARGET_NAME=$(basename "$SRC")
fi

check_deps

echo -e "${BLUE}====== project: ${YELLOW}$TARGET_NAME${BLUE} | preset: ${YELLOW}$PRESET${BLUE} ======${NC}"

pushd "$SRC_DIR" > /dev/null
BUILD_DIR="build/$PRESET"

if [[ "$DO_CLEAN" == "true" ]]; then
    run_step "cleaning build dir" rm -rf "$BUILD_DIR"
fi

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    mkdir -p "$BUILD_DIR"
    run_step "configure cmake" cmake --preset "$PRESET"
fi

run_step "build target" cmake --build --target "$TARGET_NAME" --preset "$PRESET"

if [[ "$DO_FLASH" == "true" ]]; then
    ELF="$BUILD_DIR/${TARGET_NAME}.elf"
    if [[ ! -f "$ELF" ]]; then
        echo -e "${RED}✗ flash failed: Could not find elf file at $ELF${NC}"
        exit 1
    fi

    FLASH_CMD=(STM32_Programmer_CLI --connect port="$PORT" freq="$FREQ" reset="$RESET")
    FLASH_CMD+=(--write "$ELF")
    FLASH_CMD+=(--verify)
    FLASH_CMD+=(--start)  # TODO: should this be conditional?

    run_step "flash mcu" "${FLASH_CMD[@]}"
fi

popd > /dev/null
echo -e "${GREEN}====== success ======${NC}"
