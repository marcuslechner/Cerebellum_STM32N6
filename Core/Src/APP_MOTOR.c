/*
GIM6010-8 hip motor on the CAN bus, CAN Simple protocol (manual section 4.1).
One motor for now.

Frame ID = (node_id << 5) | cmd_id, standard 11-bit, 8 data bytes, little-endian.

So far this only listens for the heartbeat, which the motor sends by itself every 100 ms
(odrv0.axis0.config.can.heartbeat_rate_ms). The motor is online while heartbeats keep arriving.

Heartbeat layout, motor firmware 0.5.12 and later:
  byte 0-3  Axis_Error  uint32
  byte 4    Axis_State  uint8
  byte 5    Flags       uint8
  byte 6    reserved
  byte 7    Life        uint8
Firmware 0.5.11 and earlier puts motor, encoder and controller flags in bytes 5, 6 and 7 instead.
*/

#include "APP_MOTOR.h"

#include "HDW_CAN.h"
#include "app_debug_log.h"

#define APP_MOTOR_NODE_SHIFT            5U
#define APP_MOTOR_NODE_MASK             0x7E0U  /* Upper 6 bits of the ID: every message from one node. */
#define APP_MOTOR_CMD_MASK              0x01FU

#define APP_MOTOR_CMD_HEARTBEAT         0x001U
#define APP_MOTOR_HEARTBEAT_LEN         8U

#define APP_MOTOR_HEARTBEAT_TIMEOUT_MS  500U    /* Five missed heartbeats at the default 100 ms. */

static APP_MOTOR_Heartbeat s_heartbeat;
static uint32_t s_last_heartbeat_ms = 0U;
static bool s_online = false;

static void app_motor_on_heartbeat(const HDW_CAN_Frame *frame);

HAL_StatusTypeDef APP_MOTOR_Init(FDCAN_HandleTypeDef *hfdcan)
{
  s_online = false;
  s_last_heartbeat_ms = HAL_GetTick();

  return HDW_CAN_Init(hfdcan, (uint16_t)(APP_MOTOR_NODE_ID << APP_MOTOR_NODE_SHIFT), APP_MOTOR_NODE_MASK);
}

void APP_MOTOR_Process(void)
{
  HDW_CAN_Frame frame;

  while (HDW_CAN_Receive(&frame))
  {
    if (((frame.id & APP_MOTOR_CMD_MASK) == APP_MOTOR_CMD_HEARTBEAT) && (frame.len == APP_MOTOR_HEARTBEAT_LEN))
    {
      app_motor_on_heartbeat(&frame);
    }
  }

  if (s_online && ((HAL_GetTick() - s_last_heartbeat_ms) > APP_MOTOR_HEARTBEAT_TIMEOUT_MS))
  {
    s_online = false;
    AppLog("Motor %u: heartbeat lost.\r\n", (unsigned int)APP_MOTOR_NODE_ID);
  }
}

bool APP_MOTOR_GetHeartbeat(APP_MOTOR_Heartbeat *heartbeat)
{
  if (heartbeat != NULL)
  {
    *heartbeat = s_heartbeat;
  }

  return s_online;
}

static void app_motor_on_heartbeat(const HDW_CAN_Frame *frame)
{
  APP_MOTOR_Heartbeat heartbeat;

  heartbeat.axis_error = (uint32_t)frame->data[0] |
                         ((uint32_t)frame->data[1] << 8) |
                         ((uint32_t)frame->data[2] << 16) |
                         ((uint32_t)frame->data[3] << 24);
  heartbeat.axis_state = frame->data[4];
  heartbeat.flags = frame->data[5];
  heartbeat.life = frame->data[7];

  /* Report the first heartbeat and any change after it, not all ten a second. */
  if ((!s_online) ||
      (heartbeat.axis_error != s_heartbeat.axis_error) ||
      (heartbeat.axis_state != s_heartbeat.axis_state) ||
      (heartbeat.flags != s_heartbeat.flags))
  {
    AppLog("Motor %u: heartbeat state=%u error=0x%08lX flags=0x%02X\r\n",
           (unsigned int)APP_MOTOR_NODE_ID,
           (unsigned int)heartbeat.axis_state,
           (unsigned long)heartbeat.axis_error,
           (unsigned int)heartbeat.flags);
  }

  s_heartbeat = heartbeat;
  s_last_heartbeat_ms = HAL_GetTick();
  s_online = true;
}
