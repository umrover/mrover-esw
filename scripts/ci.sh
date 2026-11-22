#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

zmodload zsh/parameter # track jobstates

PRESETS=()
JOBS=1
DO_STYLE=0
VERBOSE_MODE=0

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME [--preset <name>]+ [--jobs <n>] [--style] [--verbose]

Builds every project in ci.json (similar to GitHub CI workflow).

options:
  -p, --preset <name>   cmake preset to build, repeatable (default: Debug and Release)
  -j, --jobs <n>        build n projects at once (default: 1)
  -s, --style           also run \`style.sh --format --lint\`
  -v, --verbose         show full output for failures
  -h, --help            show this help message
EOF
    exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--preset)  need_value "$1" $#; PRESETS+=("$2"); shift 2 ;;
        -j|--jobs)    need_value "$1" $#; JOBS="$2"; shift 2 ;;
        -s|--style)   DO_STYLE=1; shift ;;
        -v|--verbose) VERBOSE_MODE=1; shift ;;
        -h|--help)    usage ;;
        *)            unknown_option "$1" ;;
    esac
done
((${#PRESETS[@]})) || PRESETS=(Debug Release)
[[ "$JOBS" == <-> ]] && ((JOBS >= 1)) || die "\`--jobs\` needs a positive integer, got \`$JOBS\`"

PROJECTS=(${(f)"$(grep -oE '"[^"]+"' "$ESW_ROOT/ci.json" | tr -d '"')"})

LOG_DIR=$(mktemp -d)
trap 'rm -rf "$LOG_DIR"' EXIT

# report <label> <seconds> <status> <log>
report() {
    local -r line=$(printf '%-28s %4ss' "$1" "$2")
    if (($3 == 0)); then
        pass "$line"
        return
    fi
    fail "$line"
    if ((VERBOSE_MODE)); then
        sed 's/^/      /' "$4"
    else
        tail -n 20 "$4" | sed 's/^/      /'
        printf "      %b\n" "${YELLOW}(re-run with \`--verbose\` for the full log)${NC}"
    fi
}

# build <preset> <project>
# log and "<status> <seconds>" go to LOG_DIR
build() {
    local -r key="${1}_${2//\//_}"
    local -r start=$SECONDS
    local build_status=0
    "$ESW_ROOT/scripts/build.sh" --src "$ESW_ROOT/$2" --preset "$1" > "$LOG_DIR/$key.log" 2>&1 || build_status=$?
    print -r -- "$build_status $((SECONDS - start))" > "$LOG_DIR/$key.status"
}

# build_presets <project>
# one project's presets in order
build_presets() {
    local preset
    for preset in "${PRESETS[@]}"; do
        build "$preset" "$1"
    done
}

# report_build <preset> <project>
report_build() {
    local -r key="${1}_${2//\//_}"
    local build_status seconds
    # a job that died before writing its status counts as failed
    [[ -f "$LOG_DIR/$key.status" ]] || print -r -- "1 0" > "$LOG_DIR/$key.status"
    read -r build_status seconds < "$LOG_DIR/$key.status"
    report "$2" "$seconds" "$build_status" "$LOG_DIR/$key.log"
}

if ((DO_STYLE)); then
    section "style"
    start=$SECONDS
    style_status=0
    "$ESW_ROOT/scripts/style.sh" --format --lint > "$LOG_DIR/style.log" 2>&1 || style_status=$?
    report "\`style.sh --format --lint\`" "$((SECONDS - start))" "$style_status" "$LOG_DIR/style.log"
fi

if ((JOBS == 1)); then
    for preset in "${PRESETS[@]}"; do
        section "$preset"
        for project in "${PROJECTS[@]}"; do
            build "$preset" "$project"
            report_build "$preset" "$project"
        done
    done
else
    # split for parallel builds
    if [[ -z "${CMAKE_BUILD_PARALLEL_LEVEL:-}" ]]; then
        CMAKE_BUILD_PARALLEL_LEVEL=$(($(getconf _NPROCESSORS_ONLN) / JOBS))
        ((CMAKE_BUILD_PARALLEL_LEVEL >= 1)) || CMAKE_BUILD_PARALLEL_LEVEL=1
    fi
    export CMAKE_BUILD_PARALLEL_LEVEL

    # build single project (with shared headers) first
    build_presets "${PROJECTS[1]}"
    for project in "${PROJECTS[@]:1}"; do
        while ((${#jobstates} >= JOBS)); do sleep 0.2; done
        build_presets "$project" &
    done
    wait

    for preset in "${PRESETS[@]}"; do
        section "$preset"
        for project in "${PROJECTS[@]}"; do
            report_build "$preset" "$project"
        done
    done
fi

printf "\n"
if ((FAILURES)); then
    printf "%b\n" "${RED}${BOLD}✗ $FAILURES check(s) failed, $PASSES passed${NC}"
    exit 1
fi
printf "%b\n" "${GREEN}${BOLD}✓ all $PASSES checks passed${NC}"
