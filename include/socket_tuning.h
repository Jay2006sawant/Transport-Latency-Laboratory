/* SPDX-License-Identifier: CC-BY-4.0 */
#ifndef SOCKET_TUNING_H
#define SOCKET_TUNING_H

#include "net_latency_lab.h"

int socket_apply_stream_client(int fd, const bench_config_t *cfg);
int socket_apply_dgram_client(int fd, const bench_config_t *cfg);

#endif
