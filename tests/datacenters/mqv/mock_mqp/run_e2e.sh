#!/usr/bin/env bash
# Run the MQV QDMI device end-to-end tests against the mock MQP-API, once per
# behaviour of the MQV test instance: as is; GET /v1/resources/<name> denied
# (403), so the device's fallback to the resource list is tested; AQT20- and
# MAQCS-style resources; and MAQCS as it runs jobs today (results as
# [{"counts": ...}], jobs with no_modify=false cancelled).
# Usage: tests/datacenters/mqv/mock_mqp/run_e2e.sh [test-build-dir]
#   (default: build/tests/datacenters/mqv, where CMake enables testing)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BUILD="${1:-build/tests/datacenters/mqv}"
PORT="${MQV_QDMI_E2E_PORT:-8402}"
TOKEN="mqv-secret"   # fixed token of the local mock, not a real credential
MOCK_PID=""
trap '[[ -n "$MOCK_PID" ]] && kill $MOCK_PID 2>/dev/null || true' EXIT

run_suite() {
  local label="$1"
  local resource="$2"
  local coupling_sites="$3"
  shift 3
  echo "== E2E against the mock: $label"
  uv run --quiet --with-requirements "$HERE/requirements.txt" \
    python "$HERE/mqp_mock.py" --site MQV --port "$PORT" --token "$TOKEN" "$@" \
    >"$BUILD/mqp_mock-$label.log" 2>&1 &
  MOCK_PID=$!
  for _ in $(seq 1 60); do
    curl -s -o /dev/null "http://127.0.0.1:$PORT/v1/resources" && break
    sleep 1
  done
  MQV_QDMI_E2E_BASEURL="http://127.0.0.1:$PORT/v1/" \
  MQV_QDMI_E2E_TOKEN="$TOKEN" \
  MQV_QDMI_E2E_RESOURCE="$resource" \
  MQV_QDMI_E2E_COUPLING_SITES="$coupling_sites" \
    ctest --test-dir "$BUILD" --output-on-failure -R 'MQVDeviceE2E'
  kill "$MOCK_PID" 2>/dev/null || true
  wait "$MOCK_PID" 2>/dev/null || true
  MOCK_PID=""
}

run_suite default Q4 24
run_suite deny-resource-detail Q4 24 --deny-resource-detail
# AQT20-style resource: connectivity "None", instructions {None: None}.
run_suite unrestricted A4 24
# MAQCS-style resource: "[(a, b), ...]" connectivity, 2x2 grid: 4 pairs.
run_suite tuple-connectivity G4 8
# MAQCS as it runs jobs today: results as [{"counts": {...}}], and jobs sent
# with no_modify=false cancelled (the device sends true by default).
run_suite maqcs-jobs G4 8 --result-shape maqcs --require-no-modify
