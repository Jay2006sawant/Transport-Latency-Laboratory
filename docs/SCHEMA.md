# JSON schemas

## Bench `net_latency_lab/2`

Required top-level keys: `schema`, `timestamp_utc`, `protocol`, `config`, `stats`.

`stats` includes mean, stdev, min, max, p50, p95, p99, p99.9 (`p999_ns`), `samples`, `failures`, and `histogram_ns` (12 bucket counts).

Optional `tuning` records CLI socket flags when present.

Schema `net_latency_lab/1` files remain valid for smoke checks; v2 adds histogram and p99.9.

## Manifest `net_latency_lab_manifest/2`

Host metadata: `hostname`, `kernel`, `cpu_model`, `cpufreq_governor`, `uname`, `sysctl_net_core`.
