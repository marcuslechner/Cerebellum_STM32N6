/*
GIM6010-8 hip motor on the CAN bus, CAN Simple protocol (manual section 4.1).
One motor for now.

Frame ID = (node_id << 5) | cmd_id, standard 11-bit, 8 data bytes, little-endian.

The motor sends a heartbeat by itself every 100 ms (odrv0.axis0.config.can.heartbeat_rate_ms).
The motor is online while heartbeats keep arriving.

With MOTOR_VEL_TEST the motor is run at a fixed speed (manual section 4.1.3.2). While it is online
and reports no error, every APP_MOTOR_COMMAND_PERIOD_MS:
  not in closed loop  Set_Controller_Mode (velocity control, velocity ramp), Set_Axis_State (closed loop)
  in closed loop      Set_Input_Vel
The speed ramps at odrv0.axis0.controller.config.vel_ramp_rate. An axis error is never cleared
from here, so a faulted motor stays stopped.

Heartbeat layout, motor firmware 0.5.11 and earlier (this motor reports 0.5.11):
  byte 0-3  Axis_Error       uint32
  byte 4    Axis_State       uint8
  byte 5    Motor_Flag       uint8  1 = motor error
  byte 6    Encoder_Flag     uint8  1 = encoder error
  byte 7    Controller_Flag  uint8  bit0 = controller error, bit7 = trajectory done
Firmware 0.5.12 and later puts all the flags in byte 5, reserves byte 6 and counts heartbeats in byte 7.
*/

#include "APP_MOTOR.h"

#include <string.h>

#include "HDW_CAN.h"
#include "app_config.h"
#include "app_debug_log.h"

#define APP_MOTOR_NODE_SHIFT            5U
#define APP_MOTOR_NODE_MASK             0x7E0U  /* Upper 6 bits of the ID: every message from one node. */
#define APP_MOTOR_CMD_MASK              0x01FU

#define APP_MOTOR_CMD_HEARTBEAT         0x001U
#define APP_MOTOR_HEARTBEAT_LEN         8U
#define APP_MOTOR_CMD_SET_AXIS_STATE    0x007U
#define APP_MOTOR_CMD_SET_CTRL_MODE     0x00BU
#define APP_MOTOR_CMD_SET_INPUT_VEL     0x00DU

#define APP_MOTOR_AXIS_STATE_CLOSED_LOOP  8U
#define APP_MOTOR_CONTROL_MODE_VELOCITY   2U
#define APP_MOTOR_INPUT_MODE_VEL_RAMP     2U

#define APP_MOTOR_COMMAND_PERIOD_MS     100U

#define APP_MOTOR_HEARTBEAT_TIMEOUT_MS  500U    /* Five missed heartbeats at the default 100 ms. */

static APP_MOTOR_Heartbeat s_heartbeat;
static uint32_t s_last_heartbeat_ms = 0U;
static bool s_online = false;

static void app_motor_on_heartbeat(const HDW_CAN_Frame *frame);

#if MOTOR_VEL_TEST
static uint32_t s_last_command_ms = 0U;

static void app_motor_vel_test(void);
static void app_motor_send(uint8_t cmd_id, const void *arg0, const void *arg1);
#endif

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

#if MOTOR_VEL_TEST
  app_motor_vel_test();
#endif
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
  heartbeat.motor_flag = frame->data[5];
  heartbeat.encoder_flag = frame->data[6];
  heartbeat.controller_flag = frame->data[7];

  /* Report the first heartbeat and any change after it, not all ten a second. */
  if ((!s_online) ||
      (heartbeat.axis_error != s_heartbeat.axis_error) ||
      (heartbeat.axis_state != s_heartbeat.axis_state) ||
      (heartbeat.motor_flag != s_heartbeat.motor_flag) ||
      (heartbeat.encoder_flag != s_heartbeat.encoder_flag) ||
      (heartbeat.controller_flag != s_heartbeat.controller_flag))
  {
    AppLog("Motor %u: heartbeat state=%u error=0x%08lX motor=%u encoder=%u controller=0x%02X\r\n",
           (unsigned int)APP_MOTOR_NODE_ID,
           (unsigned int)heartbeat.axis_state,
           (unsigned long)heartbeat.axis_error,
           (unsigned int)heartbeat.motor_flag,
           (unsigned int)heartbeat.encoder_flag,
           (unsigned int)heartbeat.controller_flag);
  }

  s_heartbeat = heartbeat;
  s_last_heartbeat_ms = HAL_GetTick();
  s_online = true;
}

#if MOTOR_VEL_TEST
static void app_motor_vel_test(void)
{
  uint32_t now = HAL_GetTick();

  if ((!s_online) || (s_heartbeat.axis_error != 0U) || ((now - s_last_command_ms) < APP_MOTOR_COMMAND_PERIOD_MS))
  {
    return;
  }
  s_last_command_ms = now;

  if (s_heartbeat.axis_state != APP_MOTOR_AXIS_STATE_CLOSED_LOOP)
  {
    uint32_t control_mode = APP_MOTOR_CONTROL_MODE_VELOCITY;
    uint32_t input_mode = APP_MOTOR_INPUT_MODE_VEL_RAMP;
    uint32_t axis_state = APP_MOTOR_AXIS_STATE_CLOSED_LOOP;
    uint32_t unused = 0U;

    app_motor_send(APP_MOTOR_CMD_SET_CTRL_MODE, &control_mode, &input_mode);
    app_motor_send(APP_MOTOR_CMD_SET_AXIS_STATE, &axis_state, &unused);
  }
  else
  {
    float input_vel = APP_MOTOR_TEST_OUTPUT_REV_S * APP_MOTOR_GEAR_RATIO;  /* Rotor turn/s. */
    float torque_ff = 0.0f;

    app_motor_send(APP_MOTOR_CMD_SET_INPUT_VEL, &input_vel, &torque_ff);
  }
}

/* Every command used here carries two 32-bit little-endian values, the same byte order as the MCU. */
static void app_motor_send(uint8_t cmd_id, const void *arg0, const void *arg1)
{
  HDW_CAN_Frame frame;

  frame.id = (uint16_t)((APP_MOTOR_NODE_ID << APP_MOTOR_NODE_SHIFT) | cmd_id);
  frame.len = HDW_CAN_MAX_DATA;
  memcpy(&frame.data[0], arg0, 4U);
  memcpy(&frame.data[4], arg1, 4U);

  /* A full TX FIFO drops the frame; the next period sends it again. */
  (void)HDW_CAN_Transmit(&frame);
}
#endif /* MOTOR_VEL_TEST */
