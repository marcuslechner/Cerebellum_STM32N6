#ifndef LSM6DSOX_IMU_H
#define LSM6DSOX_IMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32n6xx_hal.h"

#define LSM6DSOX_WHO_AM_I_VALUE 0x6CU

#ifndef LSM6DSOX_APP_ENABLE
#define LSM6DSOX_APP_ENABLE 1U
#endif

#ifndef LSM6DSOX_APP_USE_DRDY_IRQ
#define LSM6DSOX_APP_USE_DRDY_IRQ 0U
#endif

#ifndef LSM6DSOX_APP_POLL_PERIOD_MS
#define LSM6DSOX_APP_POLL_PERIOD_MS 10U
#endif

typedef enum
{
  LSM6DSOX_INIT_OK = 0,
  LSM6DSOX_INIT_ERR_NULL_HANDLE,
  LSM6DSOX_INIT_ERR_WHOAMI_READ,
  LSM6DSOX_INIT_ERR_WHOAMI_MISMATCH,
  LSM6DSOX_INIT_ERR_RESET_WRITE,
  LSM6DSOX_INIT_ERR_RESET_POLL_READ,
  LSM6DSOX_INIT_ERR_RESET_TIMEOUT,
  LSM6DSOX_INIT_ERR_CTRL3_WRITE,
  LSM6DSOX_INIT_ERR_CTRL1_WRITE,
  LSM6DSOX_INIT_ERR_CTRL2_WRITE,
  LSM6DSOX_INIT_ERR_INT1_WRITE
} LSM6DSOX_InitError;

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
HAL_StatusTypeDef LSM6DSOX_AppInit(SPI_HandleTypeDef *spi_handle);
HAL_StatusTypeDef LSM6DSOX_AppProcess(void);
void LSM6DSOX_AppHandleGpioExti(uint16_t GPIO_Pin);
HAL_StatusTypeDef LSM6DSOX_ReadWhoAmI(uint8_t *who_am_i);
HAL_StatusTypeDef LSM6DSOX_ProbeWhoAmI(SPI_HandleTypeDef *spi_handle, uint8_t *who_am_i);
HAL_StatusTypeDef LSM6DSOX_ReadRawSample(LSM6DSOX_RawSample *sample);
LSM6DSOX_InitError LSM6DSOX_GetLastInitError(void);
HAL_StatusTypeDef LSM6DSOX_GetLastHalStatus(void);
uint8_t LSM6DSOX_GetLastWhoAmI(void);

void LSM6DSOX_OnDrdyInterrupt(void);
uint32_t LSM6DSOX_TakeDrdyCount(void);

#ifdef __cplusplus
}
#endif

#endif /* LSM6DSOX_IMU_H */
