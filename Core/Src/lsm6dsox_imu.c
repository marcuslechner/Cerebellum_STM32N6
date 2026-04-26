#include "lsm6dsox_imu.h"

#include "main.h"

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

static HAL_StatusTypeDef lsm6dsox_write(uint8_t reg, const uint8_t *data, uint16_t len);
static HAL_StatusTypeDef lsm6dsox_read(uint8_t reg, uint8_t *data, uint16_t len);
static HAL_StatusTypeDef lsm6dsox_write_reg(uint8_t reg, uint8_t value);
static HAL_StatusTypeDef lsm6dsox_read_reg(uint8_t reg, uint8_t *value);

static void lsm6dsox_cs_select(void);
static void lsm6dsox_cs_deselect(void);

HAL_StatusTypeDef LSM6DSOX_Init(SPI_HandleTypeDef *spi_handle)
{
  uint8_t who_am_i = 0U;
  uint8_t ctrl3_c = 0U;
  uint32_t start_tick = 0U;

  if (spi_handle == NULL)
  {
    return HAL_ERROR;
  }

  s_spi = spi_handle;
  lsm6dsox_cs_deselect();
  HAL_Delay(LSM6DSOX_BOOT_DELAY_MS);

  if (LSM6DSOX_ReadWhoAmI(&who_am_i) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (who_am_i != LSM6DSOX_WHO_AM_I_VALUE)
  {
    return HAL_ERROR;
  }

  if (lsm6dsox_write_reg(LSM6DSOX_REG_CTRL3_C, LSM6DSOX_CTRL3_C_SW_RESET) != HAL_OK)
  {
    return HAL_ERROR;
  }

  start_tick = HAL_GetTick();
  do
  {
    if (lsm6dsox_read_reg(LSM6DSOX_REG_CTRL3_C, &ctrl3_c) != HAL_OK)
    {
      return HAL_ERROR;
    }
    if ((ctrl3_c & LSM6DSOX_CTRL3_C_SW_RESET) == 0U)
    {
      break;
    }
  } while ((HAL_GetTick() - start_tick) < LSM6DSOX_RESET_TIMEOUT_MS);

  if ((ctrl3_c & LSM6DSOX_CTRL3_C_SW_RESET) != 0U)
  {
    return HAL_TIMEOUT;
  }

  if (lsm6dsox_write_reg(LSM6DSOX_REG_CTRL3_C, LSM6DSOX_CTRL3_C_BDU | LSM6DSOX_CTRL3_C_IF_INC) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (lsm6dsox_write_reg(LSM6DSOX_REG_CTRL1_XL, LSM6DSOX_CTRL1_XL_833HZ_4G) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (lsm6dsox_write_reg(LSM6DSOX_REG_CTRL2_G, LSM6DSOX_CTRL2_G_833HZ_2000DPS) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Route only accel data-ready to INT1 so DRDY frequency is deterministic. */
  if (lsm6dsox_write_reg(LSM6DSOX_REG_INT1_CTRL, LSM6DSOX_INT1_DRDY_XL_ENABLE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  s_drdy_count = 0U;
  if (primask == 0U)
  {
    __enable_irq();
  }

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
