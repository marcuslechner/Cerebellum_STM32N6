#ifndef ESP_LINK_H
#define ESP_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "stm32n6xx_hal.h"

#define ESP_MSG_IMU     0x01U
#define ESP_MSG_STATUS  0x02U
#define ESP_MSG_INFO    0x03U
#define ESP_MSG_LOG     0x04U

typedef enum
{
  ESP_INFO_BATTERY_SOC = 0x01,
  ESP_INFO_ERROR       = 0x02,
  ESP_INFO_PID_PARAMS  = 0x03
} EspLink_InfoId;

void EspLink_Init(UART_HandleTypeDef *huart);
void EspLink_Process(void);
bool EspLink_SendInfo(EspLink_InfoId id, const void *data, uint8_t len);
uint32_t EspLink_GetDroppedCount(void);

#ifdef __cplusplus
}
#endif

#endif /* ESP_LINK_H */
