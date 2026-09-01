#!/usr/bin/env bash

set -euo pipefail

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"

exec uv run --quiet --project "$ESW_ROOT/tools" python "$ESW_ROOT/tools/scripts/monitor.py" "$@"
