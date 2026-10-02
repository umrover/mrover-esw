#!/bin/bash
# Prepares canalyzer's InfluxDB for the accelerometer:
#   - waits for the influxdb container (started by canalyzer) to be healthy
#   - creates accelerometer_db and grants the logger (write) and grafana (read) users access
# All statements are idempotent, safe to run on every start.
set -euo pipefail

INFLUX_CONTAINER="influxdb"
INFLUX_ADMIN_USER="admin"
INFLUX_ADMIN_PASSWORD="password"
DB_NAME="accelerometer_db"
TIMEOUT_S=120

echo ">> [Influx] Waiting for canalyzer's ${INFLUX_CONTAINER} container..."
for ((i = 0; i < TIMEOUT_S; i++)); do
    status="$(docker inspect --format '{{.State.Health.Status}}' "$INFLUX_CONTAINER" 2>/dev/null || true)"
    [ "$status" = "healthy" ] && break
    sleep 1
done

if [ "$status" != "healthy" ]; then
    echo "Error: ${INFLUX_CONTAINER} not healthy after ${TIMEOUT_S}s, is the canalyzer stack running?"
    exit 1
fi

docker exec "$INFLUX_CONTAINER" influx \
    -username "$INFLUX_ADMIN_USER" -password "$INFLUX_ADMIN_PASSWORD" \
    -execute "CREATE DATABASE ${DB_NAME}; GRANT WRITE ON ${DB_NAME} TO logger; GRANT READ ON ${DB_NAME} TO grafana"

echo ">> [Influx] ${DB_NAME} ready"
