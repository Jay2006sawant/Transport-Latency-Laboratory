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

static void *udp_server_thread(void *arg)
{
  bench_config_t *cfg = arg;
  server_run_udp(cfg->port, cfg->bind_addr, cfg->payload_bytes);
  return NULL;
}

static int udp_roundtrip(const bench_config_t *cfg, int fd, const uint8_t *msg, size_t len)
{
  struct sockaddr_in peer = {0};
  peer.sin_family = AF_INET;
  peer.sin_port = htons(cfg->port);
  inet_pton(AF_INET, cfg->connect_addr, &peer.sin_addr);
  if (sendto(fd, msg, len, 0, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
    return -1;
  }
  uint8_t buf[65536];
  socklen_t plen = sizeof(peer);
  ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &plen);
  if (n < 0 || (size_t)n != len) {
    return -1;
  }
  return memcmp(msg, buf, len) == 0 ? 0 : -1;
}

int run_udp_latency_bench(const bench_config_t *cfg, const char *json_path)
{
  g_net_server_stop = 0;
  pthread_t thr;
  if (pthread_create(&thr, NULL, udp_server_thread, (void *)cfg) != 0) {
    return 1;
  }
  sleep_ms(50);

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    g_net_server_stop = 1;
    pthread_join(thr, NULL);
    return 1;
  }
  struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  size_t len = cfg->payload_bytes < 32 ? 32 : (size_t)cfg->payload_bytes;
  uint8_t *msg = calloc(1, len);
  if (!msg) {
    close(fd);
    g_net_server_stop = 1;
    pthread_join(thr, NULL);
    return 1;
  }

  latency_stats_t st;
  stats_reset(&st);

  for (uint64_t i = 0; i < cfg->warmup + cfg->iterations; i++) {
    for (size_t j = 0; j < len; j++) {
      msg[j] = (uint8_t)((i + j) & 0xff);
    }
    uint64_t t0 = monotonic_ns();
    if (udp_roundtrip(cfg, fd, msg, len) != 0) {
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
  free(msg);

  FILE *fp = fopen(json_path, "w");
  if (!fp) {
    return 1;
  }
  json_write_bench_header(fp, "udp", cfg);
  json_write_stats_row(fp, "udp", &st, 0);
  fprintf(fp, "}\n");
  fclose(fp);
  return st.samples == 0 ? 1 : 0;
}
