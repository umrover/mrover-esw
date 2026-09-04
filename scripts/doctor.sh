#!/usr/bin/env bash

set -euo pipefail

BOLD="\033[1m"
RED="\033[1;31m"
GREEN="\033[1;32m"
BLUE="\033[1;34m"
YELLOW="\033[1;33m"
NC="\033[0m"

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"
SCRIPT_NAME=$(basename "$0")

readonly EXPECTED_GCC_VERSION="14.3"
readonly SMOKE_SRC="src/tests/logger"
readonly SMOKE_TARGET="logger"
readonly SUBMODULE_DIR="lib/stm32g4/STM32CubeG4"

if [[ "$(uname)" == "Darwin" ]]; then
    readonly ST_OPT_ROOT="/opt/ST"
    readonly CUBECLT_GLOB="STM32CubeCLT_*"
    readonly CUBEPRG_GUI="/Applications/STMicroelectronics/STM32CubeProgrammer.app/Contents/MacOs/STM32CubeProgrammer"
    readonly PROFILE_SNIPPET="/etc/mrover-esw.sh"
else
    readonly ST_OPT_ROOT="/opt/st"
    readonly CUBECLT_GLOB="stm32cubeclt_*"
    readonly CUBEPRG_GUI="/usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32CubeProgrammer"
    readonly PROFILE_SNIPPET="/etc/profile.d/mrover-esw.sh"
fi
readonly DESKTOP_DIR="$HOME/.local/share/applications"

if [[ "$(uname)" == "Darwin" ]]; then
    readonly VSCODE_BIN="/usr/local/bin/code"
else
    readonly VSCODE_BIN="/usr/bin/code"
fi

DO_BUILD=0
VERBOSE_MODE=0
FAILURES=0
WARNINGS=0

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME [--build] [--verbose]

Checks that an ESW development environment is set up correctly.

options:
  -b, --build     also run an end-to-end smoke build of ${SMOKE_SRC}
  -v, --verbose   show full command output for failures
  -h, --help      show this help message
EOF
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--build)   DO_BUILD=1; shift ;;
        -v|--verbose) VERBOSE_MODE=1; shift ;;
        -h|--help)    usage ;;
        *)            printf "%b\n" "${RED}✗ unknown option: $1${NC}" >&2; usage ;;
    esac
done

section() { printf "\n%b\n" "${BOLD}$1${NC}"; }
pass()    { printf "  %b %s\n" "${GREEN}✓${NC}" "$1"; }
note()    { printf "  %b %s\n" "${BLUE}·${NC}" "$1"; }
warn()    { printf "  %b %s\n" "${YELLOW}!${NC}" "$1"; WARNINGS=$((WARNINGS + 1)); }
fail()    { printf "  %b %s\n" "${RED}✗${NC}" "$1"; FAILURES=$((FAILURES + 1)); }

check_tool() {
    local exe="$1"
    local mode="${2:-version}"
    local path

    if ! path=$(command -v "$exe" 2>/dev/null); then
        fail "$exe not found in PATH"
        return 0
    fi

    if [[ "$mode" == "no-version" ]]; then
        pass "$(printf '%-24s %s' "$exe" "$path")"
        return 0
    fi

    local version_output version
    version_output=$(timeout 15 "$path" --version 2>&1 || true)

    version=$(printf '%s' "$version_output" \
        | sed -E 's/\x1b\[[0-9;]*[a-zA-Z]//g' \
        | tr -s '[:space:]' '\n' | grep -v '/' \
        | grep -oE '[0-9]+(\.[0-9]+)+' | head -n 1)
    [[ -n "$version" ]] || version="?"

    pass "$(printf '%-24s %-12s %s' "$exe" "$version" "$path")"
    return 0
}

check_file() {
    local label="$1" path="$2"
    if [[ -x "$path" ]]; then
        pass "$(printf '%-24s %-12s %s' "$label" "" "$path")"
    else
        warn "$label not found at $path - run ./scripts/bootstrap.sh"
    fi
}

section "build tools"
for tool in cmake ninja git uv; do check_tool "$tool"; done

section "arm toolchain"
for tool in arm-none-eabi-gcc arm-none-eabi-g++ arm-none-eabi-objcopy arm-none-eabi-size; do
    check_tool "$tool"
done

section "stm32cube tools"
check_tool STM32_Programmer_CLI
check_tool ST-LINK_gdbserver
check_tool STM32CubeMX no-version

if [[ "$(uname)" == "Linux" ]]; then
    check_file STM32CubeProgrammer "$CUBEPRG_GUI"
fi

CUBEIDE_CANDIDATES=("$ST_OPT_ROOT"/stm32cubeide_*/stm32cubeide)
CUBEIDE="${CUBEIDE_CANDIDATES[-1]}"  # newest version by name
if [[ -x "$CUBEIDE" ]]; then
    pass "$(printf '%-24s %-12s %s' "STM32CubeIDE" "$(basename "$(dirname "$CUBEIDE")" | sed 's/stm32cubeide_//')" "$CUBEIDE")"
else
    note "STM32CubeIDE not installed (optional - graphical debugging only)"
fi

if [[ "$(uname)" == "Linux" ]]; then
    section "desktop entries"
    for app in STM32CubeMX STM32CubeProgrammer; do
        entry="$DESKTOP_DIR/$app.desktop"
        if [[ ! -f "$entry" ]]; then
            warn "$app.desktop missing - re-run: ./scripts/bootstrap.sh"
            continue
        fi

        target=$(sed -n 's/^Exec=//p' "$entry" 2>/dev/null | head -n 1 || true)
        if [[ ! -x "$target" ]]; then
            warn "$app.desktop points at a missing binary: ${target:-<no Exec= line>}"
        else
            pass "$(printf '%-28s %s' "$app.desktop" "$target")"
        fi
    done

    if [[ -x "$CUBEIDE" ]]; then
        ide_entry=$(find /usr/share/applications -maxdepth 1 -name 'stm32cubeide*.desktop' 2>/dev/null | head -n 1 || true)
        if [[ -n "$ide_entry" ]]; then
            pass "$(printf '%-28s %s' "$(basename "$ide_entry")" "installed by the CubeIDE package")"
        else
            warn "STM32CubeIDE is installed but ships no launcher entry"
        fi
    fi
fi

section "editor"
if [[ ! -x "$VSCODE_BIN" ]]; then
    note "VS Code not installed at $VSCODE_BIN (optional - nothing in the build needs it)"
else
    pass "$(printf '%-24s %-12s %s' "code" "$("$VSCODE_BIN" --version 2>/dev/null | head -n 1)" "$VSCODE_BIN")"
    installed_ext=$("$VSCODE_BIN" --list-extensions 2>/dev/null | tr '[:upper:]' '[:lower:]' || true)
    while IFS= read -r ext; do
        [[ -n "$ext" ]] || continue
        if grep -qix "$ext" <<< "$installed_ext"; then
            pass "  extension $ext"
        else
            warn "  extension $ext missing - re-run: ./scripts/bootstrap.sh --tags vscode"
        fi
    done < <(sed -n '/^ *vscode_extensions:/,/^ *[a-z_]*:[^-]/p' "$ESW_ROOT/ansible/bootstrap.yml" 2>/dev/null \
        | grep -oE '^ *- *[A-Za-z0-9_-]+\.[A-Za-z0-9_-]+' | sed -E 's/^ *- *//' | tr '[:upper:]' '[:lower:]')
fi

section "style tools"
for tool in clang-format shellcheck; do check_tool "$tool"; done

section "arm toolchain provenance"
if arm_gcc=$(command -v arm-none-eabi-gcc 2>/dev/null); then
    resolved=$(realpath "$arm_gcc")
    case "$resolved" in
        "$ST_OPT_ROOT"/*)
            clt_root=$(printf '%s' "$resolved" | sed -E "s|^($ST_OPT_ROOT/[^/]+).*|\\1|")
            pass "using CubeCLT's bundled toolchain ($(basename "$clt_root"))"
            ;;
        *)
            warn "arm-none-eabi-gcc resolves outside CubeCLT: $resolved"
            warn "  falling back to the standalone toolchain; re-run ./scripts/bootstrap.sh to fix PATH"
            ;;
    esac

    clt_count=$(find "$ST_OPT_ROOT" -maxdepth 1 -name "$CUBECLT_GLOB" -type d 2>/dev/null | wc -l || true)
    if ((clt_count > 1)); then
        warn "$clt_count CubeCLT versions are installed under $ST_OPT_ROOT"
        warn "  remove the old package(s) so only one remains: dpkg -l | grep stm32cubeclt"
        warn "  and note a shell opened before the upgrade still has the old PATH"
    fi

    gcc_version=$("$arm_gcc" -dumpversion 2>/dev/null || echo "?")
    if [[ "$gcc_version" == "$EXPECTED_GCC_VERSION"* ]]; then
        pass "gcc $gcc_version matches the version CI builds with ($EXPECTED_GCC_VERSION)"
    else
        warn "gcc $gcc_version differs from the $EXPECTED_GCC_VERSION CI pins in Dockerfile.arm-gnu"
    fi
else
    fail "arm-none-eabi-gcc not found - install STM32CubeCLT via ./scripts/bootstrap.sh"
fi

section "PATH profile"
if [[ ! -f "$PROFILE_SNIPPET" ]]; then
    warn "$PROFILE_SNIPPET not found - PATH is not managed by ./scripts/bootstrap.sh"
else
    stale=0
    unsourced=0
    while IFS= read -r dir; do
        [[ -n "$dir" ]] || continue
        if [[ ! -d "$dir" ]]; then
            warn "$PROFILE_SNIPPET references a missing directory: $dir"
            stale=1
        elif [[ ":$PATH:" != *":$dir:"* ]]; then
            unsourced=1
        fi
    done < <(sed -nE 's/^_mrover_esw_(prepend|append) "([^"]*)".*/\2/p' "$PROFILE_SNIPPET")

    if ((stale)); then
        warn "  CubeCLT was likely upgraded; re-run ./scripts/bootstrap.sh --tags path-profile"
    elif ((unsourced)); then
        warn "$PROFILE_SNIPPET exists but its directories are not on your PATH"
        warn "  this shell predates the profile - open a new terminal, and if that"
        warn "  does not help, log out and back in (or: exec \$SHELL -l)"
    else
        pass "$PROFILE_SNIPPET is present, current, and sourced into this shell"
    fi
fi

if [[ -d /etc/profile.d ]]; then
    while IFS= read -r legacy; do
        warn "legacy PATH snippet $legacy also puts ST tools on PATH - delete it"
    done < <(grep -lE 'STMicroelectronics|stm32cubeclt' /etc/profile.d/*.sh 2>/dev/null \
        | grep -vE "^${PROFILE_SNIPPET}$|/cubeclt-bin-path_[^/]*\\.sh$")
fi

section "repository"
if [[ -e "$ESW_ROOT/$SUBMODULE_DIR/.git" ]]; then
    pass "submodule $SUBMODULE_DIR is initialized"
else
    fail "submodule $SUBMODULE_DIR missing - run: git submodule update --init --recursive"
fi

section "python environment"
sync_log=$(mktemp)
trap 'rm -f "$sync_log"' EXIT
if ! command -v uv > /dev/null 2>&1; then
    fail "uv not found - cannot verify tools/.venv (see 'build tools' above)"
elif uv sync --quiet --project "$ESW_ROOT/tools" --locked > "$sync_log" 2>&1; then
    pass "tools/.venv is in sync with tools/uv.lock"
else
    fail "uv sync failed - tools/uv.lock may be stale (run: uv lock --project tools)"
    sed 's/^/      /' "$sync_log"
fi

if ((DO_BUILD)); then
    section "smoke build ($SMOKE_SRC)"
    elf="$ESW_ROOT/$SMOKE_SRC/build/Debug/$SMOKE_TARGET.elf"
    build_log=$(mktemp)
    trap 'rm -f "$sync_log" "$build_log"' EXIT

    rm -rf "$ESW_ROOT/$SMOKE_SRC/build"
    printf "  %b building (from a clean tree)...\n" "${BLUE}→${NC}"
    if "$ESW_ROOT/scripts/build.sh" --src "$ESW_ROOT/$SMOKE_SRC" --preset Debug > "$build_log" 2>&1; then
        if [[ -f "$elf" ]]; then
            pass "linked $elf"
        else
            fail "build reported success but $elf is missing"
        fi
    else
        fail "build of $SMOKE_SRC failed"
        if ((VERBOSE_MODE)); then
            sed 's/^/      /' "$build_log"
        else
            tail -n 20 "$build_log" | sed 's/^/      /'
            printf "      %b\n" "${YELLOW}(re-run with --verbose for the full log)${NC}"
        fi
    fi
fi

printf "\n"
if ((FAILURES)); then
    printf "%b\n" "${RED}${BOLD}✗ $FAILURES check(s) failed, $WARNINGS warning(s)${NC}"
    exit 1
fi

if ((WARNINGS)); then
    printf "%b\n" "${YELLOW}${BOLD}✓ all required checks passed, with $WARNINGS warning(s)${NC}"
else
    printf "%b\n" "${GREEN}${BOLD}✓ all checks passed${NC}"
fi

if ((!DO_BUILD)); then
    printf "%b\n" "run with ${BOLD}--build${NC} to also verify an end-to-end firmware build"
fi
