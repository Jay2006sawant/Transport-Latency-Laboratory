/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog)
{
  fprintf(stderr,
          "Usage:\n"
          "  %s selftest\n"
          "  %s manifest --output path.json\n"
          "  %s serve tcp|udp|sctp [--port N] [--payload N] [--bind ADDR]\n"
          "  %s bench tcp|udp|sctp --output path.json [options]\n"
          "\n"
          "Bench options:\n"
          "  --port N --iterations N --warmup N --payload N\n"
          "  --connect ADDR --bind ADDR\n"
          "  --tcp-nodelay 0|1 --sndbuf N --rcvbuf N\n"
          "  --clients N --pin-cpu N\n",
          prog,
          prog,
          prog,
          prog);
}

void default_cfg(bench_config_t *cfg)
{
  *cfg = (bench_config_t){
      .warmup = 20,
      .iterations = 200,
      .payload_bytes = 512,
      .port = 19091,
      .bind_addr = "127.0.0.1",
      .connect_addr = "127.0.0.1",
      .tcp_nodelay = -1,
      .sndbuf = 0,
      .rcvbuf = 0,
      .clients = 1,
      .pin_cpu = -1,
  };
}

static int parse_common(int argc, char **argv, int start, bench_config_t *cfg)
{
  for (int i = start; i < argc; i++) {
    if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
      cfg->port = (uint16_t)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
      cfg->iterations = (uint64_t)strtoull(argv[++i], NULL, 10);
    } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
      cfg->warmup = (uint64_t)strtoull(argv[++i], NULL, 10);
    } else if (strcmp(argv[i], "--payload") == 0 && i + 1 < argc) {
      cfg->payload_bytes = (uint32_t)atoi(argv[++i]);
    } else if (strcmp(argv[i], "--bind") == 0 && i + 1 < argc) {
      cfg->bind_addr = argv[++i];
    } else if (strcmp(argv[i], "--connect") == 0 && i + 1 < argc) {
      cfg->connect_addr = argv[++i];
    } else if (strcmp(argv[i], "--tcp-nodelay") == 0 && i + 1 < argc) {
      cfg->tcp_nodelay = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--sndbuf") == 0 && i + 1 < argc) {
      cfg->sndbuf = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--rcvbuf") == 0 && i + 1 < argc) {
      cfg->rcvbuf = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--clients") == 0 && i + 1 < argc) {
      cfg->clients = (uint32_t)atoi(argv[++i]);
      if (cfg->clients < 1) {
        cfg->clients = 1;
      }
    } else if (strcmp(argv[i], "--pin-cpu") == 0 && i + 1 < argc) {
      cfg->pin_cpu = atoi(argv[++i]);
    }
  }
  return 0;
}

int run_selftest(void)
{
  bench_config_t cfg;
  default_cfg(&cfg);
  cfg.warmup = 5;
  cfg.iterations = 50;
  cfg.port = 19092;

  if (run_tcp_latency_bench(&cfg, "/tmp/net_latency_tcp_selftest.json") != 0) {
    fprintf(stderr, "TCP selftest failed\n");
    return 1;
  }
  cfg.port = 19093;
  if (run_udp_latency_bench(&cfg, "/tmp/net_latency_udp_selftest.json") != 0) {
    fprintf(stderr, "UDP selftest failed\n");
    return 1;
  }
#if defined(NET_LATENCY_HAVE_SCTP)
  cfg.port = 19094;
  if (run_sctp_latency_bench(&cfg, "/tmp/net_latency_sctp_selftest.json") != 0) {
    fprintf(stderr, "SCTP selftest failed\n");
    return 1;
  }
#endif
  printf("OK selftest tcp+udp"
#if defined(NET_LATENCY_HAVE_SCTP)
         "+sctp"
#endif
         "\n");
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }

  if (strcmp(argv[1], "selftest") == 0) {
    return run_selftest();
  }

  if (strcmp(argv[1], "manifest") == 0) {
    const char *out = "manifest.json";
    for (int i = 2; i + 1 < argc; i++) {
      if (strcmp(argv[i], "--output") == 0) {
        out = argv[i + 1];
      }
    }
    return write_platform_manifest(out);
  }

  bench_config_t cfg;
  default_cfg(&cfg);

  if (strcmp(argv[1], "serve") == 0 && argc >= 3) {
    parse_common(argc, argv, 3, &cfg);
    return run_serve(argv[2], &cfg);
  }

  if (strcmp(argv[1], "bench") == 0 && argc >= 4) {
    const char *proto = argv[2];
    const char *out = NULL;
    default_cfg(&cfg);
    for (int i = 3; i < argc; i++) {
      if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
        out = argv[++i];
      }
    }
    parse_common(argc, argv, 3, &cfg);
    if (!out) {
      usage(argv[0]);
      return 2;
    }
    if (strcmp(proto, "tcp") == 0) {
      return run_tcp_latency_bench(&cfg, out);
    }
    if (strcmp(proto, "udp") == 0) {
      return run_udp_latency_bench(&cfg, out);
    }
#if defined(NET_LATENCY_HAVE_SCTP)
    if (strcmp(proto, "sctp") == 0) {
      return run_sctp_latency_bench(&cfg, out);
    }
#endif
  }

  usage(argv[0]);
  return 2;
}
