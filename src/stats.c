/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <math.h>
#include <stdlib.h>

#define MAX_SAMPLES 200000

static double *g_samples;
static size_t g_count;
static size_t g_cap;

static int cmp_double(const void *a, const void *b)
{
  double da = *(const double *)a;
  double db = *(const double *)b;
  return (da > db) - (da < db);
}

void stats_reset(latency_stats_t *s)
{
  g_count = 0;
  if (!g_samples) {
    g_cap = MAX_SAMPLES;
    g_samples = calloc(g_cap, sizeof(double));
  }
  *s = (latency_stats_t){0};
}

void stats_add_sample(latency_stats_t *s, double ns)
{
  if (g_count < g_cap) {
    g_samples[g_count++] = ns;
  }
  s->samples++;
  if (s->samples == 1) {
    s->min_ns = s->max_ns = s->mean_ns = ns;
    return;
  }
  if (ns < s->min_ns) {
    s->min_ns = ns;
  }
  if (ns > s->max_ns) {
    s->max_ns = ns;
  }
  double delta = ns - s->mean_ns;
  s->mean_ns += delta / (double)s->samples;
}

void stats_finalize(latency_stats_t *s)
{
  if (s->samples < 2 || g_count == 0) {
    return;
  }
  double var = 0.0;
  for (size_t i = 0; i < g_count; i++) {
    double d = g_samples[i] - s->mean_ns;
    var += d * d;
  }
  s->stdev_ns = sqrt(var / (double)(g_count - 1));
  qsort(g_samples, g_count, sizeof(double), cmp_double);
  size_t i50 = (g_count - 1) * 50 / 100;
  size_t i95 = (g_count - 1) * 95 / 100;
  size_t i99 = (g_count - 1) * 99 / 100;
  s->p50_ns = g_samples[i50];
  s->p95_ns = g_samples[i95];
  s->p99_ns = g_samples[i99];
}
