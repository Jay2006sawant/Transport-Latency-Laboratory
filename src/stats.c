/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <math.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_SAMPLES 400000

static double *g_samples;
static size_t g_count;
static size_t g_cap;
static pthread_mutex_t g_stats_mu = PTHREAD_MUTEX_INITIALIZER;

static const double k_hist_edges[NET_LATENCY_HIST_BUCKETS] = {
    2000.0,   5000.0,   10000.0,  20000.0,  50000.0,  100000.0,
    200000.0, 500000.0, 1000000.0, 2000000.0, 5000000.0, 1e12,
};

static int cmp_double(const void *a, const void *b)
{
  double da = *(const double *)a;
  double db = *(const double *)b;
  return (da > db) - (da < db);
}

static void hist_add(latency_stats_t *s, double ns)
{
  for (size_t i = 0; i < NET_LATENCY_HIST_BUCKETS; i++) {
    if (ns <= k_hist_edges[i]) {
      s->histogram[i]++;
      return;
    }
  }
}

void stats_reset(latency_stats_t *s)
{
  pthread_mutex_lock(&g_stats_mu);
  g_count = 0;
  if (!g_samples) {
    g_cap = MAX_SAMPLES;
    g_samples = calloc(g_cap, sizeof(double));
  }
  *s = (latency_stats_t){0};
  pthread_mutex_unlock(&g_stats_mu);
}

void stats_add_sample(latency_stats_t *s, double ns)
{
  pthread_mutex_lock(&g_stats_mu);
  if (g_count < g_cap) {
    g_samples[g_count++] = ns;
  }
  s->samples++;
  hist_add(s, ns);
  if (s->samples == 1) {
    s->min_ns = s->max_ns = s->mean_ns = ns;
    pthread_mutex_unlock(&g_stats_mu);
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
  pthread_mutex_unlock(&g_stats_mu);
}

void stats_finalize(latency_stats_t *s)
{
  pthread_mutex_lock(&g_stats_mu);
  if (s->samples < 1 || g_count == 0) {
    pthread_mutex_unlock(&g_stats_mu);
    return;
  }
  if (s->samples < 2) {
    s->p50_ns = s->p95_ns = s->p99_ns = s->p999_ns = s->mean_ns;
    pthread_mutex_unlock(&g_stats_mu);
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
  size_t i999 = (g_count - 1) * 999 / 1000;
  s->p50_ns = g_samples[i50];
  s->p95_ns = g_samples[i95];
  s->p99_ns = g_samples[i99];
  s->p999_ns = g_samples[i999];
  pthread_mutex_unlock(&g_stats_mu);
}
