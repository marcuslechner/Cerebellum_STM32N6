#include "lsm6dsox_imu.h"

#include "main.h"

#include <stdio.h>
#include <string.h>

#define LSM6DSOX_SPI_TIMEOUT_MS            100U
#define LSM6DSOX_SPI_MAX_TRANSFER_BYTES    32U
#define LSM6DSOX_RESET_TIMEOUT_MS          100U
#define LSM6DSOX_BOOT_DELAY_MS             10U

#define LSM6DSOX_REG_INT1_CTRL             0x0DU
#define LSM6DSOX_REG_CTRL1_XL              0x10U
#define LSM6DSOX_REG_CTRL2_G               0x11U
#define LSM6DSOX_REG_CTRL3_C               0x12U
#define LSM6DSOX_REG_WHO_AM_I              0x0FU
#define LSM6DSOX_REG_OUTX_L_G              0x22U

#define LSM6DSOX_CTRL3_C_SW_RESET          0x01U
#define LSM6DSOX_CTRL3_C_IF_INC            0x04U
#define LSM6DSOX_CTRL3_C_BDU               0x40U

#define LSM6DSOX_CTRL1_XL_833HZ_4G         0x78U
#define LSM6DSOX_CTRL2_G_833HZ_2000DPS     0x7CU
#define LSM6DSOX_INT1_DRDY_XL_ENABLE       0x01U

static SPI_HandleTypeDef *s_spi = NULL;
static volatile uint32_t s_drdy_count = 0U;
static LSM6DSOX_InitError s_last_init_error = LSM6DSOX_INIT_OK;
static HAL_StatusTypeDef s_last_hal_status = HAL_OK;
static uint8_t s_last_who_am_i = 0U;

#if (LSM6DSOX_APP_ENABLE == 1U)
static LSM6DSOX_RawSample s_last_sample = {0};
static uint8_t s_has_sample = 0U;
static uint32_t s_samples_in_window = 0U;
static uint32_t s_last_poll_ms = 0U;
#endif
static uint32_t s_last_report_ms = 0U;

static HAL_StatusTypeDef lsm6dsox_write(uint8_t reg, const uint8_t *data, uint16_t len);
static HAL_StatusTypeDef lsm6dsox_read(uint8_t reg, uint8_t *data, uint16_t len);
static HAL_StatusTypeDef lsm6dsox_write_reg(uint8_t reg, uint8_t value);
static HAL_StatusTypeDef lsm6dsox_read_reg(uint8_t reg, uint8_t *value);

static void lsm6dsox_cs_select(void);
static void lsm6dsox_cs_deselect(void);
#if (LSM6DSOX_APP_ENABLE == 1U) && (LSM6DSOX_APP_USE_DRDY_IRQ == 1U)
static void lsm6dsox_app_enable_drdy_irq(void);
#endif

HAL_StatusTypeDef LSM6DSOX_AppInit(SPI_HandleTypeDef *spi_handle)
{
#if (LSM6DSOX_APP_ENABLE == 1U)
  uint8_t who_am_i = 0U;
  HAL_StatusTypeDef init_status = LSM6DSOX_Init(spi_handle);

  if (init_status != HAL_OK)
  {
    uint8_t probe_who_am_i = 0U;
    HAL_StatusTypeDef probe_status = LSM6DSOX_ProbeWhoAmI(spi_handle, &probe_who_am_i);

    printf("LSM6DSOX init failed: init=%d step=%d last_hal=%d who=0x%02X probe=%d probe_who=0x%02X\r\n",
           (int)init_status,
           (int)LSM6DSOX_GetLastInitError(),
           (int)LSM6DSOX_GetLastHalStatus(),
           LSM6DSOX_GetLastWhoAmI(),
           (int)probe_status,
           probe_who_am_i);
    return init_status;
  }

  if ((LSM6DSOX_ReadWhoAmI(&who_am_i) != HAL_OK) || (who_am_i != LSM6DSOX_WHO_AM_I_VALUE))
  {
    printf("LSM6DSOX WHO_AM_I mismatch: 0x%02X\r\n", who_am_i);
    return HAL_ERROR;
  }

#if (LSM6DSOX_APP_USE_DRDY_IRQ == 1U)
  lsm6dsox_app_enable_drdy_irq();

  uint32_t drdy_wait_start_ms = HAL_GetTick();
  uint32_t startup_drdy_count = 0U;
  while ((HAL_GetTick() - drdy_wait_start_ms) < 250U)
  {
    startup_drdy_count += LSM6DSOX_TakeDrdyCount();
    if (startup_drdy_count > 0U)
    {
      break;
    }
  }

  if (startup_drdy_count == 0U)
  {
    printf("LSM6DSOX DRDY timeout during startup.\r\n");
    return HAL_TIMEOUT;
  }
#endif

  if (LSM6DSOX_ReadRawSample(&s_last_sample) != HAL_OK)
  {
    printf("LSM6DSOX first sample read failed.\r\n");
    return HAL_ERROR;
  }

  s_has_sample = 1U;
  s_samples_in_window = 0U;
  s_last_poll_ms = HAL_GetTick();
  s_last_report_ms = s_last_poll_ms;

  printf("LSM6DSOX bring-up ok. WHO_AM_I=0x%02X\r\n", who_am_i);
  return HAL_OK;
#else
  (void)spi_handle;
  s_last_report_ms = HAL_GetTick();
  printf("IMU functionality disabled (LSM6DSOX_APP_ENABLE=0).\r\n");
  return HAL_OK;
#endif
}

HAL_StatusTypeDef LSM6DSOX_AppProcess(void)
{
  uint32_t now = HAL_GetTick();

#if (LSM6DSOX_APP_ENABLE == 1U)
#if (LSM6DSOX_APP_USE_DRDY_IRQ == 1U)
  uint32_t pending_drdy = LSM6DSOX_TakeDrdyCount();

  while (pending_drdy > 0U)
  {
    if (LSM6DSOX_ReadRawSample(&s_last_sample) != HAL_OK)
    {
      printf("LSM6DSOX sample read failed.\r\n");
      return HAL_ERROR;
    }
    s_has_sample = 1U;
    s_samples_in_window++;
    pending_drdy--;
  }
#else
  if ((now - s_last_poll_ms) >= LSM6DSOX_APP_POLL_PERIOD_MS)
  {
    s_last_poll_ms = now;
    if (LSM6DSOX_ReadRawSample(&s_last_sample) != HAL_OK)
    {
      printf("LSM6DSOX sample read failed.\r\n");
      return HAL_ERROR;
    }
    s_has_sample = 1U;
    s_samples_in_window++;
  }
#endif

  if ((now - s_last_report_ms) >= 1000U)
  {
    s_last_report_ms = now;
    HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);

    if (s_has_sample != 0U)
    {
      printf("samples/s=%lu g=[%d,%d,%d] a=[%d,%d,%d]\r\n",
             (unsigned long)s_samples_in_window,
             s_last_sample.gyro_x, s_last_sample.gyro_y, s_last_sample.gyro_z,
             s_last_sample.accel_x, s_last_sample.accel_y, s_last_sample.accel_z);
    }
    else
    {
      printf("samples/s=%lu (waiting for first sample)\r\n", (unsigned long)s_samples_in_window);
    }
    s_samples_in_window = 0U;
  }
#else
  if ((now - s_last_report_ms) >= 1000U)
  {
    s_last_report_ms = now;
    HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
    printf("IMU disabled.\r\n");
  }
#endif

  return HAL_OK;
}

void LSM6DSOX_AppHandleGpioExti(uint16_t GPIO_Pin)
{
#if (LSM6DSOX_APP_ENABLE == 1U) && (LSM6DSOX_APP_USE_DRDY_IRQ == 1U)
  if (GPIO_Pin == IMU_INT_Pin)
  {
    LSM6DSOX_OnDrdyInterrupt();
  }
#else
  (void)GPIO_Pin;
#endif
}

HAL_StatusTypeDef LSM6DSOX_Init(SPI_HandleTypeDef *spi_handle)
{
  uint8_t who_am_i = 0U;
  uint8_t ctrl3_c = 0U;
  uint32_t start_tick = 0U;

  s_last_init_error = LSM6DSOX_INIT_OK;
  s_last_hal_status = HAL_OK;
  s_last_who_am_i = 0U;

  if (spi_handle == NULL)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_NULL_HANDLE;
    s_last_hal_status = HAL_ERROR;
    return HAL_ERROR;
  }

  s_spi = spi_handle;
  lsm6dsox_cs_deselect();
  HAL_Delay(LSM6DSOX_BOOT_DELAY_MS);

  s_last_hal_status = LSM6DSOX_ReadWhoAmI(&who_am_i);
  s_last_who_am_i = who_am_i;
  if (s_last_hal_status != HAL_OK)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_WHOAMI_READ;
    return HAL_ERROR;
  }

  if (who_am_i != LSM6DSOX_WHO_AM_I_VALUE)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_WHOAMI_MISMATCH;
    s_last_hal_status = HAL_ERROR;
    return HAL_ERROR;
  }

  s_last_hal_status = lsm6dsox_write_reg(LSM6DSOX_REG_CTRL3_C, LSM6DSOX_CTRL3_C_SW_RESET);
  if (s_last_hal_status != HAL_OK)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_RESET_WRITE;
    return HAL_ERROR;
  }

  start_tick = HAL_GetTick();
  do
  {
    s_last_hal_status = lsm6dsox_read_reg(LSM6DSOX_REG_CTRL3_C, &ctrl3_c);
    if (s_last_hal_status != HAL_OK)
    {
      s_last_init_error = LSM6DSOX_INIT_ERR_RESET_POLL_READ;
      return HAL_ERROR;
    }
    if ((ctrl3_c & LSM6DSOX_CTRL3_C_SW_RESET) == 0U)
    {
      break;
    }
  } while ((HAL_GetTick() - start_tick) < LSM6DSOX_RESET_TIMEOUT_MS);

  if ((ctrl3_c & LSM6DSOX_CTRL3_C_SW_RESET) != 0U)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_RESET_TIMEOUT;
    s_last_hal_status = HAL_TIMEOUT;
    return HAL_TIMEOUT;
  }

  s_last_hal_status = lsm6dsox_write_reg(LSM6DSOX_REG_CTRL3_C, LSM6DSOX_CTRL3_C_BDU | LSM6DSOX_CTRL3_C_IF_INC);
  if (s_last_hal_status != HAL_OK)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_CTRL3_WRITE;
    return HAL_ERROR;
  }

  s_last_hal_status = lsm6dsox_write_reg(LSM6DSOX_REG_CTRL1_XL, LSM6DSOX_CTRL1_XL_833HZ_4G);
  if (s_last_hal_status != HAL_OK)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_CTRL1_WRITE;
    return HAL_ERROR;
  }

  s_last_hal_status = lsm6dsox_write_reg(LSM6DSOX_REG_CTRL2_G, LSM6DSOX_CTRL2_G_833HZ_2000DPS);
  if (s_last_hal_status != HAL_OK)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_CTRL2_WRITE;
    return HAL_ERROR;
  }

  /* Route only accel data-ready to INT1 so DRDY frequency is deterministic. */
  s_last_hal_status = lsm6dsox_write_reg(LSM6DSOX_REG_INT1_CTRL, LSM6DSOX_INT1_DRDY_XL_ENABLE);
  if (s_last_hal_status != HAL_OK)
  {
    s_last_init_error = LSM6DSOX_INIT_ERR_INT1_WRITE;
    return HAL_ERROR;
  }

  s_drdy_count = 0U;

  return HAL_OK;
}

HAL_StatusTypeDef LSM6DSOX_ReadWhoAmI(uint8_t *who_am_i)
{
  if (who_am_i == NULL)
  {
    return HAL_ERROR;
  }

  return lsm6dsox_read_reg(LSM6DSOX_REG_WHO_AM_I, who_am_i);
}

HAL_StatusTypeDef LSM6DSOX_ProbeWhoAmI(SPI_HandleTypeDef *spi_handle, uint8_t *who_am_i)
{
  if ((spi_handle == NULL) || (who_am_i == NULL))
  {
    return HAL_ERROR;
  }

  s_spi = spi_handle;
  lsm6dsox_cs_deselect();
  HAL_Delay(LSM6DSOX_BOOT_DELAY_MS);

  s_last_hal_status = lsm6dsox_read_reg(LSM6DSOX_REG_WHO_AM_I, who_am_i);
  s_last_who_am_i = *who_am_i;
  return s_last_hal_status;
}

HAL_StatusTypeDef LSM6DSOX_ReadRawSample(LSM6DSOX_RawSample *sample)
{
  uint8_t raw[12] = {0};

  if (sample == NULL)
  {
    return HAL_ERROR;
  }

  if (lsm6dsox_read(LSM6DSOX_REG_OUTX_L_G, raw, sizeof(raw)) != HAL_OK)
  {
    return HAL_ERROR;
  }

  sample->gyro_x = (int16_t)((((uint16_t)raw[1]) << 8U) | raw[0]);
  sample->gyro_y = (int16_t)((((uint16_t)raw[3]) << 8U) | raw[2]);
  sample->gyro_z = (int16_t)((((uint16_t)raw[5]) << 8U) | raw[4]);
  sample->accel_x = (int16_t)((((uint16_t)raw[7]) << 8U) | raw[6]);
  sample->accel_y = (int16_t)((((uint16_t)raw[9]) << 8U) | raw[8]);
  sample->accel_z = (int16_t)((((uint16_t)raw[11]) << 8U) | raw[10]);

  return HAL_OK;
}

void LSM6DSOX_OnDrdyInterrupt(void)
{
  s_drdy_count++;
}

uint32_t LSM6DSOX_TakeDrdyCount(void)
{
  uint32_t primask = __get_PRIMASK();
  uint32_t pending = 0U;

  __disable_irq();
  pending = s_drdy_count;
  s_drdy_count = 0U;
  if (primask == 0U)
  {
    __enable_irq();
  }

  return pending;
}

LSM6DSOX_InitError LSM6DSOX_GetLastInitError(void)
{
  return s_last_init_error;
}

HAL_StatusTypeDef LSM6DSOX_GetLastHalStatus(void)
{
  return s_last_hal_status;
}

uint8_t LSM6DSOX_GetLastWhoAmI(void)
{
  return s_last_who_am_i;
}

static HAL_StatusTypeDef lsm6dsox_write_reg(uint8_t reg, uint8_t value)
{
  return lsm6dsox_write(reg, &value, 1U);
}

static HAL_StatusTypeDef lsm6dsox_read_reg(uint8_t reg, uint8_t *value)
{
  return lsm6dsox_read(reg, value, 1U);
}

static HAL_StatusTypeDef lsm6dsox_write(uint8_t reg, const uint8_t *data, uint16_t len)
{
  uint8_t tx[LSM6DSOX_SPI_MAX_TRANSFER_BYTES + 1U] = {0};
  HAL_StatusTypeDef status = HAL_ERROR;

  if ((s_spi == NULL) || (data == NULL) || (len == 0U) || (len > LSM6DSOX_SPI_MAX_TRANSFER_BYTES))
  {
    return HAL_ERROR;
  }

  tx[0] = reg & 0x7FU;
  memcpy(&tx[1], data, len);

  lsm6dsox_cs_select();
  /* TODO: switch to SPI DMA for lower CPU overhead once IMU bring-up is stable. */
  status = HAL_SPI_Transmit(s_spi, tx, (uint16_t)(len + 1U), LSM6DSOX_SPI_TIMEOUT_MS);
  lsm6dsox_cs_deselect();

  return status;
}

static HAL_StatusTypeDef lsm6dsox_read(uint8_t reg, uint8_t *data, uint16_t len)
{
  uint8_t tx[LSM6DSOX_SPI_MAX_TRANSFER_BYTES + 1U] = {0};
  uint8_t rx[LSM6DSOX_SPI_MAX_TRANSFER_BYTES + 1U] = {0};
  HAL_StatusTypeDef status = HAL_ERROR;

  if ((s_spi == NULL) || (data == NULL) || (len == 0U) || (len > LSM6DSOX_SPI_MAX_TRANSFER_BYTES))
  {
    return HAL_ERROR;
  }

  tx[0] = reg | 0x80U;
  memset(&tx[1], 0xFF, len);

  lsm6dsox_cs_select();
  /* TODO: switch to SPI DMA for lower CPU overhead once IMU bring-up is stable. */
  status = HAL_SPI_TransmitReceive(s_spi, tx, rx, (uint16_t)(len + 1U), LSM6DSOX_SPI_TIMEOUT_MS);
  lsm6dsox_cs_deselect();

  if (status == HAL_OK)
  {
    memcpy(data, &rx[1], len);
  }

  return status;
}

static void lsm6dsox_cs_select(void)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET);
}

static void lsm6dsox_cs_deselect(void)
{
  HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET);
}

#if (LSM6DSOX_APP_ENABLE == 1U) && (LSM6DSOX_APP_USE_DRDY_IRQ == 1U)
static void lsm6dsox_app_enable_drdy_irq(void)
{
  /* Clear any stale EXTI/NVIC pending state before arming the line. */
  __HAL_GPIO_EXTI_CLEAR_IT(IMU_INT_Pin);
  HAL_NVIC_ClearPendingIRQ(EXTI12_IRQn);
  HAL_NVIC_SetPriority(EXTI12_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI12_IRQn);
}
#endif
