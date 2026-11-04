#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

# all args pass to python
esw_uv python "$TOOLS_DIR/scripts/monitor.py" "$@"
