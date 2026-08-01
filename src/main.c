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
          "  %s bench tcp|udp|sctp --output path.json [--port N] [--iterations N] [--warmup N] [--payload N]\n",
          prog,
          prog,
          prog);
}

static void default_cfg(bench_config_t *cfg)
{
  *cfg = (bench_config_t){
      .warmup = 20,
      .iterations = 200,
      .payload_bytes = 512,
      .port = 19091,
      .bind_addr = "127.0.0.1",
      .connect_addr = "127.0.0.1",
  };
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

  if (strcmp(argv[1], "bench") == 0 && argc >= 4) {
    const char *proto = argv[2];
    const char *out = NULL;
    bench_config_t cfg;
    default_cfg(&cfg);
    for (int i = 3; i < argc; i++) {
      if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
        out = argv[++i];
      } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
        cfg.port = (uint16_t)atoi(argv[++i]);
      } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
        cfg.iterations = (uint64_t)strtoull(argv[++i], NULL, 10);
      } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
        cfg.warmup = (uint64_t)strtoull(argv[++i], NULL, 10);
      } else if (strcmp(argv[i], "--payload") == 0 && i + 1 < argc) {
        cfg.payload_bytes = (uint32_t)atoi(argv[++i]);
      }
    }
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
