#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Compare two net_latency_lab JSON bench files."""

from __future__ import annotations

import json
import sys
from pathlib import Path

FIELDS = ("p50_ns", "p95_ns", "p99_ns", "p999_ns", "mean_ns")


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def pct_delta(old: float, new: float) -> float:
    if old == 0:
        return 0.0
    return (new - old) / old * 100.0


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: compare_runs.py baseline.json candidate.json", file=sys.stderr)
        return 2
    a = load(Path(sys.argv[1]))
    b = load(Path(sys.argv[2]))
    sa = a.get("stats", {})
    sb = b.get("stats", {})
    print(f"baseline protocol={a.get('protocol')} schema={a.get('schema')}")
    print(f"candidate protocol={b.get('protocol')} schema={b.get('schema')}")
    print(f"{'field':<12} {'baseline':>14} {'candidate':>14} {'delta%':>10}")
    for key in FIELDS:
        va = float(sa.get(key, 0))
        vb = float(sb.get(key, 0))
        print(f"{key:<12} {va:14.1f} {vb:14.1f} {pct_delta(va, vb):10.2f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
