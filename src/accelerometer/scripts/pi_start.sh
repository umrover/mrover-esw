#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$(realpath "$SCRIPT_DIR/..")"

"$SCRIPT_DIR/init_influx.sh"

echo ">> [Start] Starting Docker Compose (Silent Mode)..."
docker compose up -d --remove-orphans
