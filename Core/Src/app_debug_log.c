#include "app_debug_log.h"

#include "app_config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if (!UART_DEBUGGING) && BLE_TELEMETRY
#include "esp_link.h"

static char s_line[ESP_LINK_MAX_LOG_TEXT + 1U];
static size_t s_line_len = 0U;

static void app_log_send_line(size_t len);
#endif

void AppLog(const char *fmt, ...)
{
#if UART_DEBUGGING
  va_list args;

  va_start(args, fmt);
  (void)vprintf(fmt, args);
  va_end(args);
#elif BLE_TELEMETRY
  va_list args;
  int written;
  char *newline;

  va_start(args, fmt);
  written = vsnprintf(&s_line[s_line_len], sizeof(s_line) - s_line_len, fmt, args);
  va_end(args);

  if (written < 0)
  {
    return;
  }

  /* vsnprintf reports the untruncated length; anything past the end of the line buffer is lost. */
  s_line_len += (size_t)written;
  if (s_line_len > (sizeof(s_line) - 1U))
  {
    s_line_len = sizeof(s_line) - 1U;
  }

  /* One LOG frame per complete line. */
  while ((newline = memchr(s_line, '\n', s_line_len)) != NULL)
  {
    size_t line_len = (size_t)(newline - s_line);
    size_t used = line_len + 1U;

    app_log_send_line(line_len);
    s_line_len -= used;
    memmove(s_line, &s_line[used], s_line_len);
  }

  /* A full buffer with no newline goes out as it is, so the next message has room. */
  if (s_line_len == (sizeof(s_line) - 1U))
  {
    app_log_send_line(s_line_len);
    s_line_len = 0U;
  }
#else
  (void)fmt;
#endif
}

#if (!UART_DEBUGGING) && BLE_TELEMETRY
static void app_log_send_line(size_t len)
{
  if ((len > 0U) && (s_line[len - 1U] == '\r'))
  {
    len--;
  }

  if (len > 0U)
  {
    (void)EspLink_SendLog(ESP_LOG_LEVEL_INFO, s_line, (uint8_t)len);
  }
}
#endif
