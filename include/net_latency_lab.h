/* SPDX-License-Identifier: CC-BY-4.0 */
#ifndef NET_LATENCY_LAB_H
#define NET_LATENCY_LAB_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NET_LATENCY_SCHEMA_BENCH "net_latency_lab/1"
#define NET_LATENCY_SCHEMA_MANIFEST "net_latency_lab_manifest/1"

typedef struct {
  uint64_t warmup;
  uint64_t iterations;
  uint32_t payload_bytes;
  uint16_t port;
  const char *bind_addr;
  const char *connect_addr;
} bench_config_t;

typedef struct {
  double p50_ns;
  double p95_ns;
  double p99_ns;
  double mean_ns;
  double stdev_ns;
  double min_ns;
  double max_ns;
  uint64_t samples;
  uint64_t failures;
} latency_stats_t;

uint64_t monotonic_ns(void);
void sleep_ms(unsigned ms);

int run_selftest(void);
int run_tcp_latency_bench(const bench_config_t *cfg, const char *json_path);
int run_udp_latency_bench(const bench_config_t *cfg, const char *json_path);
int run_sctp_latency_bench(const bench_config_t *cfg, const char *json_path);

int write_platform_manifest(const char *output_path);

void stats_reset(latency_stats_t *s);
void stats_add_sample(latency_stats_t *s, double ns);
void stats_finalize(latency_stats_t *s);

int json_write_bench_header(FILE *fp, const char *protocol, const bench_config_t *cfg);
int json_write_stats_row(FILE *fp, const char *protocol, const latency_stats_t *st, int trailing_comma);

int server_run_tcp(uint16_t port, const char *bind_addr, uint32_t payload_bytes);
int server_run_udp(uint16_t port, const char *bind_addr, uint32_t payload_bytes);
#if defined(NET_LATENCY_HAVE_SCTP)
int server_run_sctp(uint16_t port, const char *bind_addr, uint32_t payload_bytes);
#endif

#endif
