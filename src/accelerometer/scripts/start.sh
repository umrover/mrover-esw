#!/bin/bash
# Requires the canalyzer stack (influxdb + grafana) to already be running.
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$(realpath "$SCRIPT_DIR/..")"

"$SCRIPT_DIR/init_influx.sh"

cleanup() {
    echo ""
    echo ">> [Cleanup] Shutting down accelerometer..."
    docker compose down
}

# Trap Ctrl+C (SIGINT) and EXIT
trap cleanup SIGINT EXIT

echo ">> [Start] Starting Docker Compose..."
docker compose up -d --remove-orphans

echo ">> System is running. Press Ctrl+C to stop."
sleep infinity
