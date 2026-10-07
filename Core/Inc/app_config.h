#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Build-time switches. Always define each as 0 or 1 and test with #if, never #ifdef. */

#define BLE_TELEMETRY   1   /* ESP32 link on USART3: IMU stream, INFO and LOG frames. */
#define UART_DEBUGGING  0   /* Debug messages as text on USART1 (ST-LINK virtual COM port). */
#define DEBUG_IMU       0   /* Once-a-second IMU sample report in the debug log. */

#endif /* APP_CONFIG_H */
