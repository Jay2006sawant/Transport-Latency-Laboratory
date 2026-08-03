# JSON schema `net_latency_lab/1`

Bench output is one object per run:

| Field | Type | Meaning |
|-------|------|---------|
| `schema` | string | `net_latency_lab/1` |
| `timestamp_utc` | string | ISO-8601 UTC at write time |
| `protocol` | string | `tcp`, `udp`, or `sctp` |
| `config` | object | warmup, iterations, payload, addresses, port |
| `stats` | object | RTT nanoseconds: mean, stdev, min, max, p50, p95, p99, samples, failures |

Manifest export uses `net_latency_lab_manifest/1` with host CPU, kernel, and endianness.
