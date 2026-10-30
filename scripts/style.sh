#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

FORMAT_MODE=0
LINT_MODE=0
FIX_MODE=0
VERBOSE_MODE=0
EXIT_CODE=0

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME [--format] [--lint] [--fix] [--verbose]

Checks formatting and lints C/C++, python and shell sources (format only by default).

options:
  -f, --format          check formatting (\`clang-format\`, \`ruff format\`)
  -l, --lint            lint (\`ruff check\`, \`ty\`, \`zsh -n\`, \`shellcheck\` for any bash scripts)
  -x, --fix             apply fixes instead of only reporting them
  -v, --verbose         verbose tool output
  -h, --help            show this help message
EOF
    exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -f|--format)    FORMAT_MODE=1; shift ;;
        -l|--lint)      LINT_MODE=1; shift ;;
        -x|--fix)       FIX_MODE=1; shift ;;
        -v|--verbose)   VERBOSE_MODE=1; shift ;;
        -h|--help)      usage ;;
        *)              unknown_option "$1" ;;
    esac
done

# paths relative to repository root
cd "$ESW_ROOT"

setopt null_glob

collect_cpp_files() {
    # skips symlinks
    find src lib \
        -type f \
        \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cu' -o -name '*.cuh' \) \
        ! -path '*/.venv/*' \
        ! -path 'lib/stm32g4/STM32CubeG4/*'
}
CPP_FILES=()
while IFS= read -r file; do
    CPP_FILES+=("$file")
done < <(collect_cpp_files)
readonly CPP_FILES

readonly PYTHON_DIRS=(
    tools
)
readonly SHELL_FILES=(
    scripts/*.sh
)

BASH_FILES=()
ZSH_FILES=()
for file in "${SHELL_FILES[@]}"; do
    case "$(head -n 1 "$file")" in
        *zsh) ZSH_FILES+=("$file") ;;
        *)    BASH_FILES+=("$file") ;;
    esac
done
readonly BASH_FILES ZSH_FILES

if (( ${#CPP_FILES[@]} == 0 )); then
    printf "%b\n\n" "${YELLOW}warning:${NC} no C/C++ files found!"
fi

if (( ${#PYTHON_DIRS[@]} == 0 )); then
    printf "%b\n\n" "${YELLOW}warning:${NC} no python directories found!"
fi

if (( ${#SHELL_FILES[@]} == 0 )); then
    printf "%b\n\n" "${YELLOW}warning:${NC} no shell script files found!"
fi

if ((!FIX_MODE)); then
    printf "%b\n\n" "${YELLOW}note:${NC} running in check-only mode (use \`--fix\` to apply fixes)" >&2
fi

# default behavior: if neither format nor lint is specified, run format
if ((FORMAT_MODE == 0 && LINT_MODE == 0)); then
    FORMAT_MODE=1
fi

# print a tool's version to stderr
print_version() {
    local -r name="$1"
    shift

    local version_output
    version_output=$("$@" --version 2>&1) || die "could not run \`${name}\`"

    local version
    version=$(printf '%s\n' "$version_output" | grep -oE '[0-9]+(\.[0-9]+)+' | head -n 1 || true)
    if [[ -n "$version" ]]; then
        printf "%b\n" "${GREEN}${name} version: ${version}${NC}" >&2
    else
        printf "%b\n" "${YELLOW}${name} version: found, but version string hidden${NC}" >&2
    fi
}

require clang-format
readonly CLANG_FORMAT=(clang-format)
readonly RUFF=(esw_uv ruff)
readonly TY=(esw_uv ty)
SHELLCHECK=()
if ((${#BASH_FILES[@]})); then
    require shellcheck
    SHELLCHECK=(shellcheck)
fi
readonly SHELLCHECK

print_version clang-format "${CLANG_FORMAT[@]}"
print_version ruff "${RUFF[@]}"
print_version ty "${TY[@]}"
if ((${#SHELLCHECK[@]})); then
    print_version shellcheck "${SHELLCHECK[@]}"
fi

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

        printf "%b\n" "${BOLD}formatting with ${BLUE}clang-format${NC}..."
        if "${CLANG_FORMAT[@]}" "${CLANG_FORMAT_ARGS[@]}" "${CPP_FILES[@]}"; then
            printf "%b\n" "${GREEN}done!${NC}"
        else
            EXIT_CODE=1
            printf "%b\n" "${RED}clang-format found issues${NC}"
        fi
    fi

    # ===== ruff (Python) =====
    if ((${#PYTHON_DIRS[@]})); then
        RUFF_FORMAT_ARGS=()
        if ((!FIX_MODE)); then
            RUFF_FORMAT_ARGS+=("--check")
        fi

        printf "\n"
        printf "%b\n" "${BOLD}formatting with ${BLUE}ruff${NC}..."
        if "${RUFF[@]}" format "${RUFF_ARGS[@]}" "${RUFF_FORMAT_ARGS[@]}" "${PYTHON_DIRS[@]}"; then
            printf "%b\n" "${GREEN}done!${NC}"
        else
            EXIT_CODE=1
            printf "%b\n" "${RED}ruff format found issues${NC}"
        fi
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
        printf "%b\n" "${BOLD}linting with ${BLUE}ruff${NC}..."
        if "${RUFF[@]}" check "${RUFF_ARGS[@]}" "${RUFF_CHECK_ARGS[@]}" "${PYTHON_DIRS[@]}"; then
            printf "%b\n" "${GREEN}done!${NC}"
        else
            EXIT_CODE=1
            printf "%b\n" "${RED}ruff check found issues${NC}"
        fi
    fi

    # ===== ty (Python) =====
    if ((${#PYTHON_DIRS[@]})); then
        TY_ARGS=("--respect-ignore-files")
        if ((VERBOSE_MODE)); then
            TY_ARGS+=("--verbose")
        fi

        printf "\n"
        printf "%b\n" "${BOLD}type checking with ${BLUE}ty${NC}..."
        if "${TY[@]}" check "${TY_ARGS[@]}" "${PYTHON_DIRS[@]}"; then
            printf "%b\n" "${GREEN}done!${NC}"
        else
            EXIT_CODE=1
            printf "%b\n" "${RED}ty found issues${NC}"
        fi
    fi

    # ===== shellcheck (Bash) =====
    if ((${#BASH_FILES[@]})); then
        printf "\n"
        printf "%b\n" "${BOLD}linting with ${BLUE}shellcheck${NC}..."
        if ((VERBOSE_MODE)); then
            printf "%s\n" "${BASH_FILES[@]}"
        fi
        # SC2155 is separate declaration and command.
        if "${SHELLCHECK[@]}" --exclude=SC2155 "${BASH_FILES[@]}"; then
            printf "%b\n" "${GREEN}done!${NC}"
        else
            EXIT_CODE=1
            printf "%b\n" "${RED}shellcheck found issues${NC}"
        fi
    fi

    # ===== zsh -n (Zsh, syntax only) =====
    if ((${#ZSH_FILES[@]})); then
        printf "\n"
        printf "%b\n" "${BOLD}checking syntax with ${BLUE}\`zsh -n\`${NC}..."
        ZSH_OK=1
        for file in "${ZSH_FILES[@]}"; do
            if ((VERBOSE_MODE)); then
                printf "%s\n" "$file"
            fi
            zsh -n "$file" || ZSH_OK=0
        done
        if ((ZSH_OK)); then
            printf "%b\n" "${GREEN}done!${NC}"
        else
            EXIT_CODE=1
            printf "%b\n" "${RED}zsh found syntax errors${NC}"
        fi
    fi
fi

exit "$EXIT_CODE"
