/* SPDX-License-Identifier: CC-BY-4.0 */
#include "net_latency_lab.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void json_escape(FILE *fp, const char *s)
{
  fputc('"', fp);
  if (s) {
    for (const char *p = s; *p; p++) {
      if (*p == '"' || *p == '\\') {
        fputc('\\', fp);
      }
      fputc(*p, fp);
    }
  }
  fputc('"', fp);
}

int write_platform_manifest(const char *output_path)
{
  char kernel[512] = "";
  char hostname[256] = "";
  FILE *kv = fopen("/proc/version", "r");
  if (kv) {
    fgets(kernel, sizeof(kernel), kv);
    fclose(kv);
  }
  gethostname(hostname, sizeof(hostname));

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
  fprintf(fp, "\n}\n");
  fclose(fp);
  return 0;
}
