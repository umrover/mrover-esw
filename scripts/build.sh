#!/usr/bin/env zsh

set -euo pipefail

RED="\e[1;31m"
GREEN="\e[1;32m"
BLUE="\e[1;34m"
NC="\e[0m"

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"
SRC=""
PRESET="Debug"

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
    echo -e "${RED}✗ Build failed: $SRC_DIR does not exist${NC}"
    exit 1
fi

echo -e "${BLUE}Building project $SRC target build/$PRESET...${NC}"

pushd "$SRC_DIR"

if [ ! -f "$SRC_DIR/build/$PRESET/build.ninja" ]; then
    mkdir -p "$SRC_DIR/build/$PRESET"
    run_step "Configure build" cmake --preset "$PRESET"
fi

run_step "Execute build" cmake --build --target $SRC --preset $PRESET

echo -e "${GREEN}===== BUILD SUCCESSFUL =====${NC}"

pushd

