#ifndef HDW_CAN_H
#define HDW_CAN_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32n6xx_hal.h"

#define HDW_CAN_MAX_DATA  8U  /* Classic CAN data field. */

typedef struct
{
  uint16_t id;                      /* 11-bit standard identifier. */
  uint8_t  len;                     /* Data bytes used, 0 to 8. */
  uint8_t  data[HDW_CAN_MAX_DATA];
} HDW_CAN_Frame;

/*
 * Starts the bus. Only standard data frames with (id & filter_mask) == (filter_id & filter_mask)
 * are received; everything else is rejected by the peripheral.
 */
HAL_StatusTypeDef HDW_CAN_Init(FDCAN_HandleTypeDef *hfdcan, uint16_t filter_id, uint16_t filter_mask);

/* Takes the oldest received frame. Returns false when there is none. Call from the main loop. */
bool HDW_CAN_Receive(HDW_CAN_Frame *frame);

/* Queues a standard data frame. Returns an error when the TX FIFO is full; the frame is then dropped. */
HAL_StatusTypeDef HDW_CAN_Transmit(const HDW_CAN_Frame *frame);

#endif /* HDW_CAN_H */
