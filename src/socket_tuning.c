/* SPDX-License-Identifier: CC-BY-4.0 */
#include "socket_tuning.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

int socket_apply_stream_client(int fd, const bench_config_t *cfg)
{
  if (cfg->tcp_nodelay > 0) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
  } else if (cfg->tcp_nodelay == 0) {
    int zero = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &zero, sizeof(zero));
  }
  if (cfg->sndbuf > 0) {
    int v = cfg->sndbuf;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &v, sizeof(v));
  }
  if (cfg->rcvbuf > 0) {
    int v = cfg->rcvbuf;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &v, sizeof(v));
  }
  return 0;
}

int socket_apply_dgram_client(int fd, const bench_config_t *cfg)
{
  if (cfg->sndbuf > 0) {
    int v = cfg->sndbuf;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &v, sizeof(v));
  }
  if (cfg->rcvbuf > 0) {
    int v = cfg->rcvbuf;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &v, sizeof(v));
  }
  (void)fd;
  return 0;
}
