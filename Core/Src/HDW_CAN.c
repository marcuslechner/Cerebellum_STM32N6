/*
Classic CAN on FDCAN3. Bit timing and message memory are set by MX_FDCAN3_Init() in main.c;
this file adds the receive filter, starts the bus and hands frames to the application.

Receive is polled: frames wait in the peripheral's 16-frame RX FIFO 0 until the main loop takes
them with HDW_CAN_Receive(). No FDCAN interrupt source is enabled, so FDCAN3_IT0 never fires.

Transmit goes through the peripheral's 8-frame TX FIFO and does not wait for the frame to be sent.
*/

#include "HDW_CAN.h"

#include <stddef.h>

#include "app_config.h"
#include "app_debug_log.h"

static FDCAN_HandleTypeDef *s_hfdcan = NULL;

#if DEBUG_CAN
static void hdw_can_log(const char *tag, const HDW_CAN_Frame *frame);
#endif

HAL_StatusTypeDef HDW_CAN_Init(FDCAN_HandleTypeDef *hfdcan, uint16_t filter_id, uint16_t filter_mask)
{
  FDCAN_FilterTypeDef filter = {0};
  HAL_StatusTypeDef status;

  if (hfdcan == NULL)
  {
    return HAL_ERROR;
  }

  filter.IdType = FDCAN_STANDARD_ID;
  filter.FilterIndex = 0U;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1 = filter_id;
  filter.FilterID2 = filter_mask;

  status = HAL_FDCAN_ConfigFilter(hfdcan, &filter);
  if (status != HAL_OK)
  {
    return status;
  }

  /* Anything the filter does not match, and every remote frame, is dropped. */
  status = HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT,
                                        FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);
  if (status != HAL_OK)
  {
    return status;
  }

  status = HAL_FDCAN_Start(hfdcan);
  if (status != HAL_OK)
  {
    return status;
  }

  s_hfdcan = hfdcan;
  return HAL_OK;
}

bool HDW_CAN_Receive(HDW_CAN_Frame *frame)
{
  FDCAN_RxHeaderTypeDef header;

  if ((s_hfdcan == NULL) || (frame == NULL))
  {
    return false;
  }

  if (HAL_FDCAN_GetRxFifoFillLevel(s_hfdcan, FDCAN_RX_FIFO0) == 0U)
  {
    return false;
  }

  if (HAL_FDCAN_GetRxMessage(s_hfdcan, FDCAN_RX_FIFO0, &header, frame->data) != HAL_OK)
  {
    return false;
  }

  /* Classic frames only, so the DLC code is the byte count. */
  frame->id = (uint16_t)header.Identifier;
  frame->len = (uint8_t)header.DataLength;

#if DEBUG_CAN
  hdw_can_log("RX", frame);
#endif

  return true;
}

HAL_StatusTypeDef HDW_CAN_Transmit(const HDW_CAN_Frame *frame)
{
  FDCAN_TxHeaderTypeDef header = {0};
  HAL_StatusTypeDef status;

  if ((s_hfdcan == NULL) || (frame == NULL) || (frame->len > HDW_CAN_MAX_DATA))
  {
    return HAL_ERROR;
  }

  header.Identifier = frame->id;
  header.IdType = FDCAN_STANDARD_ID;
  header.TxFrameType = FDCAN_DATA_FRAME;
  header.DataLength = frame->len;
  header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  header.BitRateSwitch = FDCAN_BRS_OFF;
  header.FDFormat = FDCAN_CLASSIC_CAN;
  header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  header.MessageMarker = 0U;

  status = HAL_FDCAN_AddMessageToTxFifoQ(s_hfdcan, &header, frame->data);

#if DEBUG_CAN
  hdw_can_log((status == HAL_OK) ? "TX" : "TX dropped", frame);
#endif

  return status;
}

#if DEBUG_CAN
/* One line per frame: tag, identifier, then the data bytes, all in hex. */
static void hdw_can_log(const char *tag, const HDW_CAN_Frame *frame)
{
  uint8_t len = (frame->len > HDW_CAN_MAX_DATA) ? HDW_CAN_MAX_DATA : frame->len;

  AppLog("CAN %s %03X:", tag, (unsigned int)frame->id);
  for (uint8_t i = 0U; i < len; i++)
  {
    AppLog(" %02X", (unsigned int)frame->data[i]);
  }
  AppLog("\r\n");
}
#endif
