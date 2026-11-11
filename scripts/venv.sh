#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME

Creates or syncs tools/.venv from tools/uv.lock.

options:
  -h, --help            show this help message
EOF
    exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage ;;
        *)         unknown_option "$1" ;;
    esac
done

require uv

uv sync --locked --project "$TOOLS_DIR"
