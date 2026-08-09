# Host tuning notes

Loopback RTT is useful for regression testing socket code paths, but absolute numbers move with governor, background load, and buffer settings.

## Before a capture run

1. Record `net_latency_lab manifest --output manifest.json`.
2. Prefer a quiet machine or note load average in the manifest hostname log.
3. On laptops, plug in power if you care about cpufreq transitions.

## Knobs this lab exposes

| Flag | Effect |
|------|--------|
| `--tcp-nodelay 1` | Disables Nagle on TCP clients |
| `--sndbuf` / `--rcvbuf` | Sets SO_SNDBUF / SO_RCVBUF when nonzero |
| `--clients N` | Runs N parallel TCP clients against one echo server |
| `--pin-cpu N` | Pins client threads to CPU N (Linux) |

## When loopback is enough

Single-host echo tests answer: "How fast can we move bytes through the local stack for a given payload?"

## When you need two hosts

Use `serve` on the receiver and `bench` with `--connect` aimed at that host. Capture manifest on **both** sides and keep clocks unrelated (RTT is still meaningful).

## Sysctl snapshot

Manifest v2 records `net.core.rmem_max` and `wmem_max`. Raise them only when you intentionally test large windows; otherwise leave defaults and document them.
