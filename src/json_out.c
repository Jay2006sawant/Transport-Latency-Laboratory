/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <stdio.h>
#include <time.h>

static void write_tuning(FILE *fp, const bench_config_t *cfg)
{
  int have = cfg->tcp_nodelay >= 0 || cfg->sndbuf > 0 || cfg->rcvbuf > 0 || cfg->clients > 1 || cfg->pin_cpu >= 0;
  if (!have) {
    return;
  }
  fprintf(fp, ",\n  \"tuning\": {\n");
  fprintf(fp, "    \"tcp_nodelay\": %d,\n", cfg->tcp_nodelay);
  fprintf(fp, "    \"sndbuf\": %d,\n", cfg->sndbuf);
  fprintf(fp, "    \"rcvbuf\": %d,\n", cfg->rcvbuf);
  fprintf(fp, "    \"clients\": %u,\n", cfg->clients);
  fprintf(fp, "    \"pin_cpu\": %d\n", cfg->pin_cpu);
  fprintf(fp, "  }");
}

int json_write_bench_header(FILE *fp, const char *protocol, const bench_config_t *cfg)
{
  time_t now = time(NULL);
  struct tm tm;
  gmtime_r(&now, &tm);
  char ts[64];
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm);
  fprintf(fp, "{\n");
  fprintf(fp, "  \"schema\": \"%s\",\n", NET_LATENCY_SCHEMA_BENCH);
  fprintf(fp, "  \"timestamp_utc\": \"%s\",\n", ts);
  fprintf(fp, "  \"protocol\": \"%s\",\n", protocol);
  fprintf(fp, "  \"config\": {\n");
  fprintf(fp, "    \"warmup\": %llu,\n", (unsigned long long)cfg->warmup);
  fprintf(fp, "    \"iterations\": %llu,\n", (unsigned long long)cfg->iterations);
  fprintf(fp, "    \"payload_bytes\": %u,\n", cfg->payload_bytes);
  fprintf(fp, "    \"port\": %u,\n", cfg->port);
  fprintf(fp, "    \"bind_addr\": \"%s\",\n", cfg->bind_addr);
  fprintf(fp, "    \"connect_addr\": \"%s\"\n", cfg->connect_addr);
  fprintf(fp, "  }");
  write_tuning(fp, cfg);
  fprintf(fp, ",\n  \"stats\": ");
  return 0;
}

int json_write_stats_row(FILE *fp, const char *protocol, const latency_stats_t *st, int trailing_comma)
{
  (void)protocol;
  (void)trailing_comma;
  fprintf(fp, "{\n");
  fprintf(fp, "    \"mean_ns\": %.2f,\n", st->mean_ns);
  fprintf(fp, "    \"stdev_ns\": %.2f,\n", st->stdev_ns);
  fprintf(fp, "    \"min_ns\": %.2f,\n", st->min_ns);
  fprintf(fp, "    \"max_ns\": %.2f,\n", st->max_ns);
  fprintf(fp, "    \"p50_ns\": %.2f,\n", st->p50_ns);
  fprintf(fp, "    \"p95_ns\": %.2f,\n", st->p95_ns);
  fprintf(fp, "    \"p99_ns\": %.2f,\n", st->p99_ns);
  fprintf(fp, "    \"p999_ns\": %.2f,\n", st->p999_ns);
  fprintf(fp, "    \"samples\": %llu,\n", (unsigned long long)st->samples);
  fprintf(fp, "    \"failures\": %llu,\n", (unsigned long long)st->failures);
  fprintf(fp, "    \"histogram_ns\": [");
  for (size_t i = 0; i < NET_LATENCY_HIST_BUCKETS; i++) {
    fprintf(fp, "%llu%s", (unsigned long long)st->histogram[i], i + 1 < NET_LATENCY_HIST_BUCKETS ? ", " : "");
  }
  fprintf(fp, "]\n  }\n");
  return 0;
}
