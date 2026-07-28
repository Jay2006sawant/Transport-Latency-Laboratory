/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <arpa/inet.h>
#include <pthread.h>
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

int run_tcp_latency_bench(const bench_config_t *cfg, const char *json_path)
{
  g_net_server_stop = 0;
  pthread_t thr;
  if (pthread_create(&thr, NULL, tcp_server_thread, (void *)cfg) != 0) {
    return 1;
  }
  sleep_ms(80);

  int fd = -1;
  for (int attempt = 0; attempt < 30 && fd < 0; attempt++) {
    if (tcp_connect(cfg, &fd) == 0) {
      break;
    }
    sleep_ms(20);
  }
  if (fd < 0) {
    g_net_server_stop = 1;
    pthread_join(thr, NULL);
    return 1;
  }

  size_t len = (size_t)cfg->payload_bytes;
  if (len < 16) {
    len = 16;
  }
  uint8_t *msg = calloc(1, len);
  if (!msg) {
    close(fd);
    g_net_server_stop = 1;
    pthread_join(thr, NULL);
    return 1;
  }
  for (size_t i = 0; i < len; i++) {
    msg[i] = (uint8_t)(i & 0xff);
  }

  latency_stats_t st;
  stats_reset(&st);

  for (uint64_t i = 0; i < cfg->warmup + cfg->iterations; i++) {
    uint64_t t0 = monotonic_ns();
    if (tcp_roundtrip(fd, msg, len) != 0) {
      st.failures++;
      continue;
    }
    uint64_t t1 = monotonic_ns();
    if (i >= cfg->warmup) {
      stats_add_sample(&st, (double)(t1 - t0));
    }
  }
  stats_finalize(&st);

  close(fd);
  g_net_server_stop = 1;
  pthread_join(thr, NULL);

  FILE *fp = fopen(json_path, "w");
  if (!fp) {
    free(msg);
    return 1;
  }
  json_write_bench_header(fp, "tcp", cfg);
  json_write_stats_row(fp, "tcp", &st, 0);
  fprintf(fp, "}\n");
  fclose(fp);
  free(msg);
  return st.samples == 0 ? 1 : 0;
}
