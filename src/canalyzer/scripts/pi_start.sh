#!/bin/bash
set -e

# -------------------------------------------------------
# Setup Directory
# -------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$(realpath "$SCRIPT_DIR/..")"

echo ">> [Start] Starting Docker Compose (Silent Mode)..."

docker compose up -d --remove-orphans