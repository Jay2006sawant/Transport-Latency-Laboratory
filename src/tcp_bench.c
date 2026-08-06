/* SPDX-License-Identifier: CC-BY-4.0 */
#define _GNU_SOURCE
#include "net_latency_lab.h"
#include "socket_tuning.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

extern volatile int g_net_server_stop;

static void *tcp_server_thread(void *arg)
{
  bench_config_t *cfg = arg;
  server_run_tcp(cfg->port, cfg->bind_addr, cfg->payload_bytes);
  return NULL;
}

static int tcp_connect(const bench_config_t *cfg, int *out_fd)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return -1;
  }
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg->port);
  inet_pton(AF_INET, cfg->connect_addr, &addr.sin_addr);
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }
  socket_apply_stream_client(fd, cfg);
  *out_fd = fd;
  return 0;
}

static int tcp_roundtrip(int fd, const uint8_t *msg, size_t len)
{
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, msg + sent, len - sent, 0);
    if (n <= 0) {
      return -1;
    }
    sent += (size_t)n;
  }
  uint8_t buf[65536];
  size_t got = 0;
  while (got < len) {
    ssize_t n = recv(fd, buf + got, len - got, 0);
    if (n <= 0) {
      return -1;
    }
    got += (size_t)n;
  }
  return memcmp(msg, buf, len) == 0 ? 0 : -1;
}

typedef struct {
  bench_config_t cfg;
  const uint8_t *msg;
  size_t len;
  latency_stats_t *st;
  uint64_t warmup;
  uint64_t iterations;
} tcp_client_ctx_t;

static void pin_self(int cpu)
{
  if (cpu < 0) {
    return;
  }
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET((unsigned)cpu, &set);
  pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
}

static void *tcp_client_thread(void *arg)
{
  tcp_client_ctx_t *c = arg;
  pin_self(c->cfg.pin_cpu);

  int fd = -1;
  for (int attempt = 0; attempt < 30 && fd < 0; attempt++) {
    if (tcp_connect(&c->cfg, &fd) == 0) {
      break;
    }
    sleep_ms(20);
  }
  if (fd < 0) {
    return NULL;
  }

  for (uint64_t i = 0; i < c->warmup + c->iterations; i++) {
    uint64_t t0 = monotonic_ns();
    if (tcp_roundtrip(fd, c->msg, c->len) != 0) {
      c->st->failures++;
      continue;
    }
    uint64_t t1 = monotonic_ns();
    if (i >= c->warmup) {
      stats_add_sample(c->st, (double)(t1 - t0));
    }
  }
  close(fd);
  return NULL;
}

static void split_work(uint64_t total, uint32_t clients, uint32_t idx, uint64_t *out)
{
  uint64_t base = total / clients;
  uint64_t extra = total % clients;
  *out = base + (idx < extra ? 1 : 0);
}

int run_tcp_latency_bench(const bench_config_t *cfg, const char *json_path)
{
  uint32_t nclients = cfg->clients ? cfg->clients : 1;
  g_net_server_stop = 0;
  pthread_t thr;
  bench_config_t scfg = *cfg;
  if (pthread_create(&thr, NULL, tcp_server_thread, &scfg) != 0) {
    return 1;
  }
  sleep_ms(100);

  size_t len = (size_t)cfg->payload_bytes;
  if (len < 16) {
    len = 16;
  }
  uint8_t *msg = calloc(1, len);
  if (!msg) {
    g_net_server_stop = 1;
    pthread_join(thr, NULL);
    return 1;
  }
  for (size_t i = 0; i < len; i++) {
    msg[i] = (uint8_t)(i & 0xff);
  }

  latency_stats_t st;
  stats_reset(&st);

  pthread_t *threads = calloc(nclients, sizeof(pthread_t));
  tcp_client_ctx_t *ctxs = calloc(nclients, sizeof(tcp_client_ctx_t));
  if (!threads || !ctxs) {
    free(msg);
    free(threads);
    free(ctxs);
    g_net_server_stop = 1;
    pthread_join(thr, NULL);
    return 1;
  }

  for (uint32_t i = 0; i < nclients; i++) {
    uint64_t w = 0;
    uint64_t it = 0;
    split_work(cfg->warmup, nclients, i, &w);
    split_work(cfg->iterations, nclients, i, &it);
    ctxs[i] = (tcp_client_ctx_t){.cfg = *cfg, .msg = msg, .len = len, .st = &st, .warmup = w, .iterations = it};
    pthread_create(&threads[i], NULL, tcp_client_thread, &ctxs[i]);
  }
  for (uint32_t i = 0; i < nclients; i++) {
    pthread_join(threads[i], NULL);
  }

  stats_finalize(&st);
  g_net_server_stop = 1;
  pthread_join(thr, NULL);

  FILE *fp = fopen(json_path, "w");
  if (!fp) {
    free(msg);
    free(threads);
    free(ctxs);
    return 1;
  }
  bench_config_t out_cfg = *cfg;
  out_cfg.clients = nclients;
  json_write_bench_header(fp, "tcp", &out_cfg);
  json_write_stats_row(fp, "tcp", &st, 0);
  fprintf(fp, "}\n");
  fclose(fp);
  free(msg);
  free(threads);
  free(ctxs);
  return st.samples == 0 ? 1 : 0;
}
