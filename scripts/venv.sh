#!/usr/bin/env bash

set -euo pipefail

ESW_ROOT="$(dirname "$(dirname "$(realpath "$0")")")"

uv sync --project "$ESW_ROOT/tools"
