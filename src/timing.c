/* SPDX-License-Identifier: CC-BY-4.0 */
#include "timing.h"

#include <time.h>

uint64_t monotonic_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void sleep_ms(unsigned ms)
{
  struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000l};
  nanosleep(&ts, NULL);
}
