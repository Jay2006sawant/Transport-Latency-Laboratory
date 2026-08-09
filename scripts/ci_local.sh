#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
RESULTS="${ROOT}/results"
mkdir -p "${RESULTS}"

cmake -S "${ROOT}" -B "${BUILD}" -GNinja
cmake --build "${BUILD}"
ctest --test-dir "${BUILD}" --output-on-failure

"${BUILD}/net_latency_lab" bench tcp --warmup 10 --iterations 100 --payload 256 \
  --output "${BUILD}/smoke_tcp.json"
python3 "${ROOT}/python/validate_json.py" "${BUILD}/smoke_tcp.json"

"${BUILD}/net_latency_lab" bench udp --warmup 10 --iterations 100 --payload 256 \
  --port 19095 --output "${RESULTS}/smoke_udp.json"
python3 "${ROOT}/python/validate_json.py" "${RESULTS}/smoke_udp.json"

if "${BUILD}/net_latency_lab" bench sctp --warmup 5 --iterations 30 --payload 128 \
  --port 19096 --output "${RESULTS}/smoke_sctp.json" 2>/dev/null; then
  python3 "${ROOT}/python/validate_json.py" "${RESULTS}/smoke_sctp.json"
fi

python3 "${ROOT}/python/run_sweep.py" "${ROOT}/manifests/default_sweep.json" "${BUILD}/net_latency_lab"
for f in "${ROOT}/results/sweep"/*.json; do
  [[ "$f" == *index.json ]] && continue
  python3 "${ROOT}/python/validate_json.py" "$f"
done

python3 "${ROOT}/python/compare_runs.py" \
  "${ROOT}/results/sweep/00_tcp_p64.json" \
  "${ROOT}/results/sweep/01_tcp_p64.json"

python3 "${ROOT}/python/report_html.py" "${RESULTS}/report.html" \
  "${BUILD}/smoke_tcp.json" "${RESULTS}/smoke_udp.json"

cp "${BUILD}/smoke_tcp.json" "${RESULTS}/smoke_tcp.json"
echo "Local CI passed. See ${RESULTS}/"
