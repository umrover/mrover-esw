#!/usr/bin/env bash

set -euo pipefail

RED="\e[1;31m"
GREEN="\e[1;32m"
BLUE="\e[1;34m"
YELLOW="\e[1;33m"
NC="\e[0m"

shopt -s nullglob globstar
GLOBIGNORE="*venv*/*"

collect_cpp_files() {
    # skips symlinks
    find src lib \
        -type f \
        \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cu' -o -name '*.cuh' \) \
        ! -path '*/venv/*'
}
mapfile -t CPP_FILES < <(collect_cpp_files)
readonly CPP_FILES

readonly PYTHON_DIRS=(
    tools
)
readonly SHELL_FILES=(
    scripts/*.sh
)

if (( ${#CPP_FILES[@]} == 0 )); then
    printf "%b\n\n" "${YELLOW}warning:${NC} no C/C++ files found!"
fi

if (( ${#PYTHON_DIRS[@]} == 0 )); then
    printf "%b\n\n" "${YELLOW}warning:${NC} no python directories found!"
fi

if (( ${#SHELL_FILES[@]} == 0 )); then
    printf "%b\n\n" "${YELLOW}warning:${NC} no shell script files found!"
fi


# =========================
# Argument parsing
# =========================
FORMAT_MODE=0
LINT_MODE=0
FIX_MODE=0
VERBOSE_MODE=0

for arg in "$@"; do
    case "$arg" in
    --format)
        FORMAT_MODE=1
        ;;
    --lint)
        LINT_MODE=1
        ;;
    --fix)
        FIX_MODE=1
        ;;
    --verbose)
        VERBOSE_MODE=1
        ;;
    *)
        printf "%b\n" "${RED}[Error] Unknown argument: ${arg}${NC}" >&2
        printf "%s\n" "Usage: ${0##*/} [--format] [--lint] [--fix] [--verbose]" >&2
        exit 1
        ;;
    esac
done

if ((!FIX_MODE)); then
    printf "%b\n\n" "${YELLOW}note:${NC} running in check-only mode (use --fix to apply fixes)" >&2
fi

# default behavior: if neither format nor lint is specified, run format
if ((FORMAT_MODE == 0 && LINT_MODE == 0)); then
    FORMAT_MODE=1
fi

find_executable() {
    local -r executable="$1"
    local -r version_pattern="${2-}"

    local path
    if ! path=$(command -v "$executable" 2>/dev/null); then
        printf "%b\n" "${RED}error: could not find ${executable}${NC}" >&2
        exit 1
    fi

    if [[ -n "$version_pattern" ]]; then
        if ! "$path" --version 2>&1 | grep -q "$version_pattern"; then
            printf "%b\n" \
                "${RED}error: wrong ${executable} version (expected something matching '${version_pattern}')${NC}" >&2
            exit 1
        fi
    fi

    printf '%s\n' "$path"
}

readonly CLANG_FORMAT_PATH=$(find_executable clang-format-21 21.1)
readonly RUFF_PATH=$(find_executable ruff 0.14)
readonly SHELLCHECK_PATH=$(find_executable shellcheck)

RUFF_ARGS=("--respect-gitignore")
if ((VERBOSE_MODE)); then
    RUFF_ARGS+=("--verbose")
fi

# =========================
# FORMAT MODE
# =========================
if ((FORMAT_MODE)); then
    # ===== clang-format (C/C++) =====
    if ((${#CPP_FILES[@]})); then
        CLANG_FORMAT_ARGS=(
            "--style=file"
        )
        if ((!FIX_MODE)); then
            CLANG_FORMAT_ARGS+=("--dry-run" "--Werror")
        else
            CLANG_FORMAT_ARGS+=("-i")
        fi

        if ((VERBOSE_MODE)); then
            CLANG_FORMAT_ARGS+=("--verbose")
        fi

        printf "%b\n" "formatting with ${BLUE}clang-format${NC}..."
        "${CLANG_FORMAT_PATH}" "${CLANG_FORMAT_ARGS[@]}" "${CPP_FILES[@]}"
        printf "%b\n" "${GREEN}done!${NC}"
    fi

    # ===== ruff (Python) =====
    if ((${#PYTHON_DIRS[@]})); then
        RUFF_FORMAT_ARGS=()
        if ((!FIX_MODE)); then
            RUFF_FORMAT_ARGS+=("--check")
        fi

        printf "\n"
        printf "%b\n" "formatting with ${BLUE}ruff${NC}..."
        "${RUFF_PATH}" format "${RUFF_ARGS[@]}" "${RUFF_FORMAT_ARGS[@]}" "${PYTHON_DIRS[@]}"
        printf "%b\n" "${GREEN}done!${NC}"
    fi
fi

# =========================
# LINT MODE
# =========================
if ((LINT_MODE)); then
    # ===== ruff (Python) =====
    if ((${#PYTHON_DIRS[@]})); then
        RUFF_CHECK_ARGS=()
        if ((!FIX_MODE)); then
            RUFF_CHECK_ARGS+=("--no-fix")
        else
            RUFF_CHECK_ARGS+=("--fix")
        fi

        printf "\n"
        printf "%b\n" "linting with ${BLUE}ruff${NC}..."
        "${RUFF_PATH}" check "${RUFF_ARGS[@]}" "${RUFF_CHECK_ARGS[@]}" "${PYTHON_DIRS[@]}"
        printf "%b\n" "${GREEN}done!${NC}"
    fi

    # ===== shellcheck (Bash) =====
    if ((${#SHELL_FILES[@]})); then
        printf "\n"
        printf "%b\n" "linting with ${BLUE}shellcheck${NC}..."
        if ((${#SHELL_FILES[@]})); then
            # SC2155 is separate declaration and command.
            "${SHELLCHECK_PATH}" --exclude=SC2155 "${SHELL_FILES[@]}"
        fi
        printf "%b\n" "${GREEN}done!${NC}"
    fi
fi
