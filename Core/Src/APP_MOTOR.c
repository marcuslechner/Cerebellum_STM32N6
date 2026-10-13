/*
GIM6010-8 hip motor on the CAN bus, CAN Simple protocol (manual section 4.1).
One motor for now.

Frame ID = (node_id << 5) | cmd_id, standard 11-bit, 8 data bytes, little-endian.

The motor sends a heartbeat by itself every 100 ms (odrv0.axis0.config.can.heartbeat_rate_ms).
The motor is online while heartbeats keep arriving.

APP_MOTOR_SetPosition() sets the position to go to (manual section 4.1.3.3). While the motor is
online, every APP_MOTOR_COMMAND_PERIOD_MS:
  error reported                       Clear_Errors, at most every APP_MOTOR_CLEAR_ERRORS_PERIOD_MS
  closed loop, mode not set from here  Set_Axis_State (idle)
  idle                                 Set_Traj_Vel_Limit (APP_MOTOR_TRAJ_OUTPUT_RPM),
                                       Set_Controller_Mode (position control, trapezoidal trajectory),
                                       Set_Axis_State (closed loop)
  closed loop, mode set from here      Set_Input_Pos, once per target
The driver keeps its control mode across an MCU reset, so a motor found already in closed loop may
still be in velocity control. It is taken to idle first: entering closed loop from idle starts the
position setpoint at the present position, so the move begins without a jump.
The move cruises at APP_MOTOR_TRAJ_OUTPUT_RPM and follows odrv0.axis0.trap_traj.config for
accel_limit and decel_limit. Set_Input_Pos is sent once because each one makes the driver plan
the trajectory again.

With MOTOR_SWEEP_TEST the target alternates between APP_MOTOR_SWEEP_MIN_OUTPUT_TURNS and
APP_MOTOR_SWEEP_MAX_OUTPUT_TURNS. The next target is set when the driver reports the trajectory
done and the position from Get_Encoder_Estimates (sent by the motor every 10 ms,
odrv0.axis0.config.can.encoder_rate_ms) is at the present target.

With MOTOR_VEL_TEST the motor is instead run at a fixed speed (manual section 4.1.3.2) and
APP_MOTOR_SetPosition() has no effect:
  not in closed loop  Set_Controller_Mode (velocity control, velocity ramp), Set_Axis_State (closed loop)
  in closed loop      Set_Input_Vel
The speed ramps at odrv0.axis0.controller.config.vel_ramp_rate. An axis error is not cleared
in this mode, so a faulted motor stays stopped.

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
#define APP_MOTOR_CMD_ENCODER_ESTIMATES 0x009U
#define APP_MOTOR_CMD_SET_CTRL_MODE     0x00BU
#define APP_MOTOR_CMD_SET_INPUT_POS      0x00CU
#define APP_MOTOR_CMD_SET_INPUT_VEL     0x00DU
#define APP_MOTOR_CMD_SET_TRAJ_VEL_LIMIT  0x011U
#define APP_MOTOR_CMD_CLEAR_ERRORS      0x018U

#define APP_MOTOR_CONTROLLER_FLAG_ERROR      0x01U
#define APP_MOTOR_CONTROLLER_FLAG_TRAJ_DONE  0x80U

#define APP_MOTOR_AXIS_STATE_IDLE         1U
#define APP_MOTOR_AXIS_STATE_CLOSED_LOOP  8U
#define APP_MOTOR_CONTROL_MODE_VELOCITY   2U
#define APP_MOTOR_CONTROL_MODE_POSITION   3U
#define APP_MOTOR_INPUT_MODE_VEL_RAMP     2U
#define APP_MOTOR_INPUT_MODE_TRAP_TRAJ    5U

#define APP_MOTOR_COMMAND_PERIOD_MS     100U
#define APP_MOTOR_CLEAR_ERRORS_PERIOD_MS  1000U
#define APP_MOTOR_SWEEP_TOLERANCE_TURNS   0.25f  /* Rotor turns from the target that count as arrived. */

#define APP_MOTOR_HEARTBEAT_TIMEOUT_MS  500U    /* Five missed heartbeats at the default 100 ms. */

static APP_MOTOR_Heartbeat s_heartbeat;
static uint32_t s_last_heartbeat_ms = 0U;
static bool s_online = false;

static uint32_t s_last_command_ms = 0U;

static void app_motor_on_heartbeat(const HDW_CAN_Frame *frame);
static HAL_StatusTypeDef app_motor_send(uint8_t cmd_id, const void *arg0, const void *arg1);

#if MOTOR_VEL_TEST
static void app_motor_vel_test(void);
#else
static float s_target_rotor_turns = 0.0f;
static bool s_target_set = false;   /* APP_MOTOR_SetPosition() has been called. */
static bool s_target_sent = false;  /* The driver has the present target. */
static bool s_mode_set = false;     /* The driver was put in position control from idle. */
static float s_rotor_turns = 0.0f;  /* Last Pos_Estimate from Get_Encoder_Estimates. */
static uint32_t s_last_clear_ms = 0U;

static void app_motor_position(void);
#if MOTOR_SWEEP_TEST
static bool s_sweep_to_max = false;

static void app_motor_sweep_test(void);
#endif
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
#if !MOTOR_VEL_TEST
    else if (((frame.id & APP_MOTOR_CMD_MASK) == APP_MOTOR_CMD_ENCODER_ESTIMATES) && (frame.len == HDW_CAN_MAX_DATA))
    {
      memcpy(&s_rotor_turns, &frame.data[0], 4U);  /* Pos_Estimate, float32 little-endian. */
    }
#endif
  }

  if (s_online && ((HAL_GetTick() - s_last_heartbeat_ms) > APP_MOTOR_HEARTBEAT_TIMEOUT_MS))
  {
    s_online = false;
#if !MOTOR_VEL_TEST
    /* The motor may have been power cycled, which puts it back in its saved control mode. */
    s_mode_set = false;
    s_target_sent = false;
#endif
    AppLog("Motor %u: heartbeat lost.\r\n", (unsigned int)APP_MOTOR_NODE_ID);
  }

#if MOTOR_VEL_TEST
  app_motor_vel_test();
#else
#if MOTOR_SWEEP_TEST
  app_motor_sweep_test();
#endif
  app_motor_position();
#endif
}

void APP_MOTOR_SetPosition(float output_turns)
{
#if MOTOR_VEL_TEST
  (void)output_turns;
#else
  s_target_rotor_turns = output_turns * APP_MOTOR_GEAR_RATIO;
  s_target_set = true;
  s_target_sent = false;
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

    (void)app_motor_send(APP_MOTOR_CMD_SET_CTRL_MODE, &control_mode, &input_mode);
    (void)app_motor_send(APP_MOTOR_CMD_SET_AXIS_STATE, &axis_state, &unused);
  }
  else
  {
    float input_vel = APP_MOTOR_TEST_OUTPUT_REV_S * APP_MOTOR_GEAR_RATIO;  /* Rotor turn/s. */
    float torque_ff = 0.0f;

    (void)app_motor_send(APP_MOTOR_CMD_SET_INPUT_VEL, &input_vel, &torque_ff);
  }
}
#else
static void app_motor_position(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t unused = 0U;

  if ((!s_target_set) || (!s_online) || ((now - s_last_command_ms) < APP_MOTOR_COMMAND_PERIOD_MS))
  {
    return;
  }
  s_last_command_ms = now;

  if ((s_heartbeat.axis_error != 0U) || (s_heartbeat.motor_flag != 0U) || (s_heartbeat.encoder_flag != 0U) ||
      ((s_heartbeat.controller_flag & APP_MOTOR_CONTROLLER_FLAG_ERROR) != 0U))
  {
    /* The driver went to idle on the error; once it reports none it is enabled again below. */
    if ((now - s_last_clear_ms) >= APP_MOTOR_CLEAR_ERRORS_PERIOD_MS)
    {
      s_last_clear_ms = now;
      AppLog("Motor %u: clearing errors.\r\n", (unsigned int)APP_MOTOR_NODE_ID);
      (void)app_motor_send(APP_MOTOR_CMD_CLEAR_ERRORS, &unused, &unused);
    }
  }
  else if (s_heartbeat.axis_state == APP_MOTOR_AXIS_STATE_IDLE)
  {
    uint32_t control_mode = APP_MOTOR_CONTROL_MODE_POSITION;
    uint32_t input_mode = APP_MOTOR_INPUT_MODE_TRAP_TRAJ;
    uint32_t axis_state = APP_MOTOR_AXIS_STATE_CLOSED_LOOP;
    float traj_vel_limit = APP_MOTOR_TRAJ_OUTPUT_RPM * APP_MOTOR_GEAR_RATIO / 60.0f;  /* Rotor turn/s. */

    s_target_sent = false;
    s_mode_set = (app_motor_send(APP_MOTOR_CMD_SET_TRAJ_VEL_LIMIT, &traj_vel_limit, &unused) == HAL_OK) &&
                 (app_motor_send(APP_MOTOR_CMD_SET_CTRL_MODE, &control_mode, &input_mode) == HAL_OK);
    if (s_mode_set)
    {
      (void)app_motor_send(APP_MOTOR_CMD_SET_AXIS_STATE, &axis_state, &unused);
    }
  }
  else if (s_heartbeat.axis_state != APP_MOTOR_AXIS_STATE_CLOSED_LOOP)
  {
    /* Calibrating or starting up: wait for idle. */
  }
  else if (!s_mode_set)
  {
    uint32_t axis_state = APP_MOTOR_AXIS_STATE_IDLE;

    (void)app_motor_send(APP_MOTOR_CMD_SET_AXIS_STATE, &axis_state, &unused);
  }
  else if (!s_target_sent)
  {
    /* Bytes 4-7 are Vel_FF and Torque_FF, two int16, both zero. */
    if (app_motor_send(APP_MOTOR_CMD_SET_INPUT_POS, &s_target_rotor_turns, &unused) == HAL_OK)
    {
      s_target_sent = true;
      AppLog("Motor %u: position target sent, %ld/1000 rotor turns.\r\n",
             (unsigned int)APP_MOTOR_NODE_ID,
             (long)(s_target_rotor_turns * 1000.0f));
    }
  }
}

#if MOTOR_SWEEP_TEST
static void app_motor_sweep_test(void)
{
  float error_turns = s_rotor_turns - s_target_rotor_turns;

  if (s_target_set &&
      ((!s_online) || (!s_target_sent) ||
       ((s_heartbeat.controller_flag & APP_MOTOR_CONTROLLER_FLAG_TRAJ_DONE) == 0U) ||
       (error_turns > APP_MOTOR_SWEEP_TOLERANCE_TURNS) || (error_turns < -APP_MOTOR_SWEEP_TOLERANCE_TURNS)))
  {
    return;
  }

  s_sweep_to_max = !s_sweep_to_max;
  APP_MOTOR_SetPosition(s_sweep_to_max ? APP_MOTOR_SWEEP_MAX_OUTPUT_TURNS : APP_MOTOR_SWEEP_MIN_OUTPUT_TURNS);
}
#endif
#endif /* MOTOR_VEL_TEST */

/* Every command used here carries two 32-bit little-endian values, the same byte order as the MCU. */
static HAL_StatusTypeDef app_motor_send(uint8_t cmd_id, const void *arg0, const void *arg1)
{
  HDW_CAN_Frame frame;

  frame.id = (uint16_t)((APP_MOTOR_NODE_ID << APP_MOTOR_NODE_SHIFT) | cmd_id);
  frame.len = HDW_CAN_MAX_DATA;
  memcpy(&frame.data[0], arg0, 4U);
  memcpy(&frame.data[4], arg1, 4U);

  /* A full TX FIFO drops the frame; the caller sends it again in a later period. */
  return HDW_CAN_Transmit(&frame);
}
