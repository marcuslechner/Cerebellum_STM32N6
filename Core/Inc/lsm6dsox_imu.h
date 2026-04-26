#ifndef LSM6DSOX_IMU_H
#define LSM6DSOX_IMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32n6xx_hal.h"

#define LSM6DSOX_WHO_AM_I_VALUE 0x6CU

typedef struct
{
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
} LSM6DSOX_RawSample;

HAL_StatusTypeDef LSM6DSOX_Init(SPI_HandleTypeDef *spi_handle);
HAL_StatusTypeDef LSM6DSOX_ReadWhoAmI(uint8_t *who_am_i);
HAL_StatusTypeDef LSM6DSOX_ReadRawSample(LSM6DSOX_RawSample *sample);

void LSM6DSOX_OnDrdyInterrupt(void);
uint32_t LSM6DSOX_TakeDrdyCount(void);

#ifdef __cplusplus
}
#endif

#endif /* LSM6DSOX_IMU_H */
