#ifndef ESP_LINK_H
#define ESP_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32n6xx_hal.h"

#define ESP_MSG_IMU     0x01U
#define ESP_MSG_STATUS  0x02U
#define ESP_MSG_INFO    0x03U
#define ESP_MSG_LOG     0x04U

#define ESP_LOG_LEVEL_INFO     0x00U /* The only LOG level so far. */
#define ESP_LINK_MAX_LOG_TEXT  96U   /* Longest LOG text; longer messages are truncated. */

typedef enum
{
  ESP_INFO_BATTERY_SOC = 0x01,
  ESP_INFO_ERROR       = 0x02,
  ESP_INFO_PID_PARAMS  = 0x03
} EspLink_InfoId;

typedef struct __attribute__((packed))
{
  uint32_t sample_idx;   /* IMU sample counter; time = idx / 833 Hz */
  int16_t  gyro_x;       /* raw LSB, same as LSM6DSOX_RawSample */
  int16_t  gyro_y;
  int16_t  gyro_z;
  int16_t  accel_x;
  int16_t  accel_y;
  int16_t  accel_z;
} EspLink_ImuPayload;    /* 16 bytes, 22 on the wire */

void EspLink_Init(UART_HandleTypeDef *huart);
void EspLink_Process(void);
bool EspLink_SendInfo(EspLink_InfoId id, const void *data, uint8_t len);
bool EspLink_SendLog(uint8_t level, const char *text, uint8_t len);
uint32_t EspLink_GetDroppedCount(void);

#endif /* ESP_LINK_H */
