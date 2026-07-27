/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <stdio.h>
#include <time.h>

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
  fprintf(fp, "  },\n");
  fprintf(fp, "  \"stats\": ");
  return 0;
}

int json_write_stats_row(FILE *fp, const char *protocol, const latency_stats_t *st, int trailing_comma)
{
  (void)protocol;
  fprintf(fp, "{\n");
  fprintf(fp, "    \"mean_ns\": %.2f,\n", st->mean_ns);
  fprintf(fp, "    \"stdev_ns\": %.2f,\n", st->stdev_ns);
  fprintf(fp, "    \"min_ns\": %.2f,\n", st->min_ns);
  fprintf(fp, "    \"max_ns\": %.2f,\n", st->max_ns);
  fprintf(fp, "    \"p50_ns\": %.2f,\n", st->p50_ns);
  fprintf(fp, "    \"p95_ns\": %.2f,\n", st->p95_ns);
  fprintf(fp, "    \"p99_ns\": %.2f,\n", st->p99_ns);
  fprintf(fp, "    \"samples\": %llu,\n", (unsigned long long)st->samples);
  fprintf(fp, "    \"failures\": %llu\n", (unsigned long long)st->failures);
  fprintf(fp, "  }%s\n", trailing_comma ? "," : "");
  return 0;
}
