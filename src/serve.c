/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>

extern volatile int g_net_server_stop;

static void on_signal(int sig)
{
  (void)sig;
  g_net_server_stop = 1;
}

int run_serve(const char *proto, const bench_config_t *cfg)
{
  g_net_server_stop = 0;
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  fprintf(stderr, "serving %s on %s:%u (Ctrl+C to stop)\n", proto, cfg->bind_addr, cfg->port);

  if (strcmp(proto, "tcp") == 0) {
    return server_run_tcp(cfg->port, cfg->bind_addr, cfg->payload_bytes);
  }
  if (strcmp(proto, "udp") == 0) {
    return server_run_udp(cfg->port, cfg->bind_addr, cfg->payload_bytes);
  }
#if defined(NET_LATENCY_HAVE_SCTP)
  if (strcmp(proto, "sctp") == 0) {
    return server_run_sctp(cfg->port, cfg->bind_addr, cfg->payload_bytes);
  }
#endif
  return 1;
}
