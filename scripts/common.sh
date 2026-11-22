#!/usr/bin/env zsh

# ZSH_ARGZERO is the calling script even inside this sourced file
ESW_ROOT="${ZSH_ARGZERO:A:h:h}"
TOOLS_DIR="$ESW_ROOT/tools"
SCRIPT_NAME="${ZSH_ARGZERO:t}"

# colors and formats
BOLD="\033[1m"
RED="\033[1;31m"
GREEN="\033[1;32m"
BLUE="\033[1;34m"
YELLOW="\033[1;33m"
NC="\033[0m"

die()  { printf "%b\n" "${RED}✗ error: $*${NC}" >&2; exit 1; }
info() { printf "%b\n" "${BLUE}$*${NC}"; }
ok()   { printf "%b\n" "${GREEN}✓ $*${NC}"; }

require() {
    local cmd
    for cmd in "$@"; do
        command -v "$cmd" > /dev/null 2>&1 || die "\`$cmd\` not found in PATH"
    done
}

# in an argument case: need_value "$1" $#
need_value() {
    (($2 >= 2)) || die "\`$1\` needs a value"
}

# in an argument case: *) unknown_option "$1" ;;  (calls script's usage)
unknown_option() {
    printf "%b\n" "${RED}✗ unknown option: \`$1\`${NC}" >&2
    usage 1 >&2
}

# an invalid invocation: usage_error "<message>"  (calls script's usage)
usage_error() {
    printf "%b\n" "${RED}✗ error: $*${NC}" >&2
    usage 1 >&2
}

# run python/python tool pinned to tools/uv.lock
esw_uv() {
    require uv
    uv run --quiet --locked --project "$TOOLS_DIR" "$@"
}

# checklist reporting
PASSES=0
WARNINGS=0
FAILURES=0
section() { printf "\n%b\n" "${BOLD}$1${NC}"; }
pass()    { printf "  %b %s\n" "${GREEN}✓${NC}" "$1"; PASSES=$((PASSES + 1)); }
note()    { printf "  %b %s\n" "${BLUE}·${NC}" "$1"; }
warn()    { printf "  %b %s\n" "${YELLOW}!${NC}" "$1"; WARNINGS=$((WARNINGS + 1)); }
fail()    { printf "  %b %s\n" "${RED}✗${NC}" "$1"; FAILURES=$((FAILURES + 1)); }
