#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Build a simple HTML table from bench JSON files."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: report_html.py out.html file1.json [file2.json ...]", file=sys.stderr)
        return 2
    out = Path(sys.argv[1])
    rows = []
    for arg in sys.argv[2:]:
        data = json.loads(Path(arg).read_text(encoding="utf-8"))
        st = data.get("stats", {})
        cfg = data.get("config", {})
        rows.append(
            {
                "file": Path(arg).name,
                "protocol": data.get("protocol", ""),
                "payload": cfg.get("payload_bytes", 0),
                "p50_us": st.get("p50_ns", 0) / 1000.0,
                "p99_us": st.get("p99_ns", 0) / 1000.0,
                "p999_us": st.get("p999_ns", 0) / 1000.0,
                "samples": st.get("samples", 0),
            }
        )

    html = [
        "<!DOCTYPE html>",
        "<html><head><meta charset='utf-8'><title>transport latency report</title>",
        "<style>body{font-family:sans-serif;margin:2rem}table{border-collapse:collapse}",
        "td,th{border:1px solid #ccc;padding:6px 10px}</style></head><body>",
        "<h1>Transport latency summary</h1>",
        "<table><tr><th>file</th><th>proto</th><th>payload</th>",
        "<th>p50 (us)</th><th>p99 (us)</th><th>p99.9 (us)</th><th>samples</th></tr>",
    ]
    for r in rows:
        html.append(
            f"<tr><td>{r['file']}</td><td>{r['protocol']}</td><td>{r['payload']}</td>"
            f"<td>{r['p50_us']:.1f}</td><td>{r['p99_us']:.1f}</td><td>{r['p999_us']:.1f}</td>"
            f"<td>{r['samples']}</td></tr>"
        )
    html += ["</table></body></html>"]
    out.write_text("\n".join(html), encoding="utf-8")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
