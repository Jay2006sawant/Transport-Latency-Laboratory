#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Run a JSON sweep manifest through net_latency_lab."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: run_sweep.py manifest.json path/to/net_latency_lab", file=sys.stderr)
        return 2
    manifest = Path(sys.argv[1])
    binary = Path(sys.argv[2])
    spec = json.loads(manifest.read_text(encoding="utf-8"))
    outdir = Path(spec.get("outdir", "results/sweep"))
    outdir.mkdir(parents=True, exist_ok=True)

    index = []
    for i, case in enumerate(spec.get("cases", [])):
        proto = case["protocol"]
        out = outdir / f"{i:02d}_{proto}_p{case.get('payload_bytes', 512)}.json"
        cmd = [
            str(binary),
            "bench",
            proto,
            "--output",
            str(out),
            "--warmup",
            str(case.get("warmup", 10)),
            "--iterations",
            str(case.get("iterations", 100)),
            "--payload",
            str(case.get("payload_bytes", 512)),
            "--port",
            str(case.get("port", 19110 + i)),
        ]
        if "tcp_nodelay" in case:
            cmd += ["--tcp-nodelay", str(case["tcp_nodelay"])]
        if case.get("clients", 1) > 1:
            cmd += ["--clients", str(case["clients"])]
        print("run", " ".join(cmd))
        subprocess.check_call(cmd)
        index.append({"file": str(out), "case": case})

    (outdir / "index.json").write_text(json.dumps(index, indent=2), encoding="utf-8")
    print(f"wrote {len(index)} runs under {outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
