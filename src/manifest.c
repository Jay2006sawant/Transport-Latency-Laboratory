/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

static void json_escape(FILE *fp, const char *s)
{
  fputc('"', fp);
  if (s) {
    for (const char *p = s; *p; p++) {
      if (*p == '"' || *p == '\\' || *p == '\n') {
        fputc('\\', fp);
        if (*p == '\n') {
          fputc('n', fp);
          continue;
        }
      }
      fputc(*p, fp);
    }
  }
  fputc('"', fp);
}

static void read_line_file(const char *path, char *buf, size_t buflen)
{
  FILE *f = fopen(path, "r");
  if (!f) {
    buf[0] = '\0';
    return;
  }
  if (!fgets(buf, (int)buflen, f)) {
    buf[0] = '\0';
  }
  fclose(f);
  size_t n = strlen(buf);
  while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
    buf[--n] = '\0';
  }
}

static void read_cpu_model(char *buf, size_t buflen)
{
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (!f) {
    buf[0] = '\0';
    return;
  }
  char line[512];
  buf[0] = '\0';
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "model name", 10) == 0) {
      char *colon = strchr(line, ':');
      if (colon) {
        strncpy(buf, colon + 2, buflen - 1);
        buf[buflen - 1] = '\0';
        break;
      }
    }
  }
  fclose(f);
}

static long read_sysctl_long(const char *path)
{
  char tmp[64];
  read_line_file(path, tmp, sizeof(tmp));
  if (!tmp[0]) {
    return -1;
  }
  return strtol(tmp, NULL, 10);
}

int write_platform_manifest(const char *output_path)
{
  char kernel[512] = "";
  char hostname[256] = "";
  char governor[64] = "";
  char cpu_model[256] = "";
  struct utsname uts;

  read_line_file("/proc/version", kernel, sizeof(kernel));
  gethostname(hostname, sizeof(hostname));
  read_line_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", governor, sizeof(governor));
  read_cpu_model(cpu_model, sizeof(cpu_model));
  uname(&uts);

  long rmem = read_sysctl_long("/proc/sys/net/core/rmem_max");
  long wmem = read_sysctl_long("/proc/sys/net/core/wmem_max");

  FILE *fp = fopen(output_path, "w");
  if (!fp) {
    return 1;
  }
  fprintf(fp, "{\n  \"schema\": ");
  json_escape(fp, NET_LATENCY_SCHEMA_MANIFEST);
  fprintf(fp, ",\n  \"hostname\": ");
  json_escape(fp, hostname);
  fprintf(fp, ",\n  \"kernel\": ");
  json_escape(fp, kernel);
  fprintf(fp, ",\n  \"cpu_model\": ");
  json_escape(fp, cpu_model);
  fprintf(fp, ",\n  \"cpufreq_governor\": ");
  json_escape(fp, governor[0] ? governor : "unknown");
  fprintf(fp, ",\n  \"uname\": {\"sysname\": ");
  json_escape(fp, uts.sysname);
  fprintf(fp, ", \"machine\": ");
  json_escape(fp, uts.machine);
  fprintf(fp, ", \"release\": ");
  json_escape(fp, uts.release);
  fprintf(fp, "},\n  \"sysctl_net_core\": {\n");
  fprintf(fp, "    \"rmem_max\": %ld,\n", rmem);
  fprintf(fp, "    \"wmem_max\": %ld\n", wmem);
  fprintf(fp, "  }\n}\n");
  fclose(fp);
  return 0;
}
