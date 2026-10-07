#ifndef APP_DEBUG_LOG_H
#define APP_DEBUG_LOG_H

/*
 * printf-style debug message. Where it goes is set in app_config.h:
 *   UART_DEBUGGING = 1                     text on USART1
 *   UART_DEBUGGING = 0, BLE_TELEMETRY = 1  LOG frames on the ESP32 link, one per line
 *   both 0                                 discarded
 * End every message with "\n"; a line may be built up over several calls.
 */
void AppLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* APP_DEBUG_LOG_H */
