#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Validate net_latency_lab JSON output."""

from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED = {"schema", "timestamp_utc", "protocol", "config", "stats"}


def validate(path: Path) -> list[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    errs: list[str] = []
    if not REQUIRED.issubset(data):
        errs.append(f"missing keys {REQUIRED - data.keys()}")
    if not str(data.get("schema", "")).startswith("net_latency_lab/"):
        errs.append("bad schema")
    st = data.get("stats", {})
    if st.get("samples", 0) < 1:
        errs.append("no samples")
    if st.get("p50_ns", 0) <= 0:
        errs.append("bad p50")
    if str(data.get("schema", "")).startswith("net_latency_lab/2"):
        if "p999_ns" not in st:
            errs.append("missing p999 in v2")
    return errs


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: validate_json.py file.json", file=sys.stderr)
        return 2
    for arg in sys.argv[1:]:
        errs = validate(Path(arg))
        if errs:
            print(f"FAIL {arg}: {errs}")
            return 1
        print(f"OK {arg}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
