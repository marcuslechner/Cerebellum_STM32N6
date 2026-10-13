#ifndef APP_MOTOR_H
#define APP_MOTOR_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32n6xx_hal.h"

/* Must match odrv0.axis0.config.can.node_id on the motor (0 to 63, factory default 0). */
#define APP_MOTOR_NODE_ID  1U

typedef struct
{
  uint32_t axis_error;  /* odrv0.axis0.error; 0 = no error. */
  uint8_t  axis_state;  /* odrv0.axis0.current_state; 1 = idle, 8 = closed loop control. */
  uint8_t  flags;       /* bit0 motor, bit1 encoder, bit2 controller, bit3 system error; bit7 trajectory done. */
  uint8_t  life;        /* Counts up by one per heartbeat. */
} APP_MOTOR_Heartbeat;

HAL_StatusTypeDef APP_MOTOR_Init(FDCAN_HandleTypeDef *hfdcan);
void APP_MOTOR_Process(void);

/* Copies the last heartbeat. Returns true while heartbeats are arriving. */
bool APP_MOTOR_GetHeartbeat(APP_MOTOR_Heartbeat *heartbeat);

#endif /* APP_MOTOR_H */
