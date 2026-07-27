/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

volatile int g_net_server_stop = 0;

static int set_reuse(int fd)
{
  int one = 1;
  return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
}

static void set_recv_timeout_ms(int fd, int ms)
{
  struct timeval tv = {.tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static size_t bench_payload_len(uint32_t payload_bytes)
{
  size_t len = (size_t)payload_bytes;
  return len < 16 ? 16 : len;
}

static int echo_full(int fd, uint8_t *buf, size_t len)
{
  size_t got = 0;
  while (got < len && !g_net_server_stop) {
    ssize_t n = recv(fd, buf + got, len - got, 0);
    if (n > 0) {
      got += (size_t)n;
      continue;
    }
    if (n == 0) {
      return -1;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    return -1;
  }
  if (got != len) {
    return -1;
  }
  size_t sent = 0;
  while (sent < len && !g_net_server_stop) {
    ssize_t n = send(fd, buf + sent, len - sent, 0);
    if (n <= 0) {
      return -1;
    }
    sent += (size_t)n;
  }
  return 0;
}

int server_run_tcp(uint16_t port, const char *bind_addr, uint32_t payload_bytes)
{
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return -1;
  }
  set_reuse(listen_fd);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, bind_addr, &addr.sin_addr);

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(listen_fd);
    return -1;
  }
  if (listen(listen_fd, 1) < 0) {
    close(listen_fd);
    return -1;
  }

  while (!g_net_server_stop) {
    struct timeval tv = {.tv_sec = 0, .tv_usec = 200000};
    setsockopt(listen_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int client = accept(listen_fd, NULL, NULL);
    if (client < 0) {
      continue;
    }
    set_recv_timeout_ms(client, 500);
    size_t msg_len = bench_payload_len(payload_bytes);
    uint8_t buf[65536];
    while (!g_net_server_stop) {
      if (echo_full(client, buf, msg_len) != 0) {
        break;
      }
    }
    close(client);
  }
  close(listen_fd);
  return 0;
}

int server_run_udp(uint16_t port, const char *bind_addr, uint32_t payload_bytes)
{
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    return -1;
  }
  set_reuse(fd);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, bind_addr, &addr.sin_addr);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }
  set_recv_timeout_ms(fd, 500);
  (void)payload_bytes;

  while (!g_net_server_stop) {
    uint8_t buf[65536];
    struct sockaddr_in peer = {0};
    socklen_t plen = sizeof(peer);
    ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &plen);
    if (n <= 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      break;
    }
    sendto(fd, buf, (size_t)n, 0, (struct sockaddr *)&peer, plen);
  }
  close(fd);
  return 0;
}

#if defined(NET_LATENCY_HAVE_SCTP)
#include <netinet/sctp.h>

int server_run_sctp(uint16_t port, const char *bind_addr, uint32_t payload_bytes)
{
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
  if (fd < 0) {
    return -1;
  }
  set_reuse(fd);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, bind_addr, &addr.sin_addr);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }
  if (listen(fd, 1) < 0) {
    close(fd);
    return -1;
  }

  while (!g_net_server_stop) {
    struct timeval tv = {.tv_sec = 0, .tv_usec = 200000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int client = accept(fd, NULL, NULL);
    if (client < 0) {
      continue;
    }
    set_recv_timeout_ms(client, 500);
    size_t msg_len = bench_payload_len(payload_bytes);
    uint8_t buf[65536];
    while (!g_net_server_stop) {
      if (echo_full(client, buf, msg_len) != 0) {
        break;
      }
    }
    close(client);
  }
  close(fd);
  return 0;
}
#endif
