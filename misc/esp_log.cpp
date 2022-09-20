

#include "esp_log.h"
#include <stdio.h>
#include <stdarg.h>

void ESP_LOGI(const char* tag, const char* fmt, ...)
{
  va_list argp;
  char    buf[160];

  va_start(argp, fmt);
  vsprintf(buf, fmt, argp);
  printf("INFO [%s] %s\n", tag, buf);
  va_end(argp);
}

void ESP_LOGW(const char* tag, const char* fmt, ...)
{
  va_list argp;
  char    buf[160];

  va_start(argp, fmt);
  vsprintf(buf, fmt, argp);
  printf("WARN [%s] %s\n", tag, buf);
  va_end(argp);
}

void ESP_LOGE(const char* tag, const char* fmt, ...)
{
  va_list argp;
  char    buf[160];

  va_start(argp, fmt);
  vsprintf(buf, fmt, argp);
  printf("ERROR [%s] %s\n", tag, buf);
  va_end(argp);
}
