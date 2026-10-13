#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Build-time switches. Always define each as 0 or 1 and test with #if, never #ifdef. */

#define BLE_TELEMETRY   0   /* ESP32 link on USART3: IMU stream, INFO and LOG frames. */
#define UART_DEBUGGING  1   /* Debug messages as text on USART1 (ST-LINK virtual COM port). */
#define DEBUG_IMU       0   /* Once-a-second IMU sample report in the debug log. */
#define DEBUG_CAN       1   /* Every CAN frame sent and received in the debug log. */
#define MOTOR_VEL_TEST  0   /* Spin the motor at APP_MOTOR_TEST_OUTPUT_REV_S as soon as it is online. */
#define MOTOR_SWEEP_TEST 1  /* Move back and forth between the APP_MOTOR_SWEEP_ positions. Needs MOTOR_VEL_TEST 0. */

#endif /* APP_CONFIG_H */
