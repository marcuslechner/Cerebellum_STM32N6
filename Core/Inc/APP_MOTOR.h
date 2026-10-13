#ifndef APP_MOTOR_H
#define APP_MOTOR_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32n6xx_hal.h"

/* Must match odrv0.axis0.config.can.node_id on the motor (0 to 63, factory default 0). */
#define APP_MOTOR_NODE_ID  1U

/* The driver works in rotor turns; the output shaft turns once per APP_MOTOR_GEAR_RATIO rotor turns. */
#define APP_MOTOR_GEAR_RATIO  8.0f

/* MOTOR_VEL_TEST speed at the output shaft, rev/s. Positive is the driver's positive direction. */
#define APP_MOTOR_TEST_OUTPUT_REV_S  1.0f

/* Top speed of a move to a position, in RPM at the output shaft. */
#define APP_MOTOR_TRAJ_OUTPUT_RPM  120.0f

/* MOTOR_SWEEP_TEST end positions, in output shaft turns from the driver's zero. */
#define APP_MOTOR_SWEEP_MIN_OUTPUT_TURNS  (-50.0f)
#define APP_MOTOR_SWEEP_MAX_OUTPUT_TURNS  50.0f

typedef struct
{
  uint32_t axis_error;       /* odrv0.axis0.error; 0 = no error. */
  uint8_t  axis_state;       /* odrv0.axis0.current_state; 1 = idle, 8 = closed loop control. */
  uint8_t  motor_flag;       /* 1 = odrv0.axis0.motor.error is non-zero. */
  uint8_t  encoder_flag;     /* 1 = odrv0.axis0.encoder.error is non-zero. */
  uint8_t  controller_flag;  /* bit0 = odrv0.axis0.controller.error is non-zero; bit7 = trajectory done. */
} APP_MOTOR_Heartbeat;

HAL_StatusTypeDef APP_MOTOR_Init(FDCAN_HandleTypeDef *hfdcan);
void APP_MOTOR_Process(void);

/*
 * Sets the position the motor should go to, in output shaft turns from the driver's zero.
 * Returns at once; APP_MOTOR_Process() puts the motor in position control and sends the target.
 * Errors the motor reports are cleared and it is enabled again. The latest call wins.
 * No effect with MOTOR_VEL_TEST; overridden by MOTOR_SWEEP_TEST.
 */
void APP_MOTOR_SetPosition(float output_turns);

/* Copies the last heartbeat. Returns true while heartbeats are arriving. */
bool APP_MOTOR_GetHeartbeat(APP_MOTOR_Heartbeat *heartbeat);

#endif /* APP_MOTOR_H */
