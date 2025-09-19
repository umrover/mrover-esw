#!/usr/bin/env zsh

set -euo pipefail

RED="\e[1;31m"
GREEN="\e[1;32m"
BLUE="\e[1;34m"
NC="\e[0m"

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"
SRC=""
PRESET="Debug"
PORT="SWD"
FREQ=4000
RESET="HWrst"

usage() {
    echo -e "${BLUE}Usage: $0 -s <src> -p <preset>${NC}"
    echo -e "${BLUE}       $0 --src <src> --preset <preset>${NC}"
    exit 1
}

run_step() {
    desc="$1"
    shift
    echo -e "${BLUE}${desc}...${NC}"
    if "$@"; then
        echo -e "${GREEN}✓ Success: ${desc}${NC}"
    else
        echo -e "${RED}✗ Failed: ${desc}${NC}"
        exit 1
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -s|--src)
            SRC="$2"
            shift 2
            ;;
        -p|--preset)
            PRESET="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}✗ Unknown option: $1${NC}"
            usage
            ;;
    esac
done

if [[ -z "$SRC" ]]; then
    echo -e "${RED}✗ Error: -s/--src required${NC}"
    usage
fi

SRC_DIR="$ESW_ROOT/src/$SRC"
if [ ! -d "$SRC_DIR" ]; then
    echo -e "${RED}✗ Flash failed: $SRC_DIR does not exist${NC}"
    exit 1
fi

echo -e "${BLUE}Flashing project $SRC...${NC}"

pushd "$SRC_DIR"

ELF="$SRC_DIR/build/$PRESET/$SRC.elf"
if [ ! -f "$ELF" ]; then
    echo -e "${RED}✗ Flash failed: $ELF does not exist${NC}"
    exit 1
fi

# TODO(eric): why does this not hard reset (reset still needs to be hit before running)?
run_step "Flash MCU" STM32_Programmer_CLI --connect port="$PORT" freq="$FREQ" reset="$RESET" --write "$ELF"

echo -e "${GREEN}===== FLASH SUCCESSFUL =====${NC}"

pushd

