/*
transmit and receive data from the BLE link using UART3, 
has DMA on transmit 
first baudrate of 115200, can be changed to 921600 for faster throughput


Inputs
can get data from the esp such as commands to control the robot, squat, stand, change speed of motors, change PID gains, etc.
Processing
first establish a uart link at 115200 baud, then can change to 921600 for faster throughput
for streaming data, we can have to split the buffer, and one goes out while the other is being filled, to avoid losing data
headroom is reserved so that stream frames are refused once the fill buffer passes 80% to allow or status frames, that way the stream can never crowd out a status update

the frames are mine to control:

#define ESP_MSG_IMU     0x01U
#define ESP_MSG_STATUS  0x02U
#define ESP_MSG_INFO  0x03U
#define ESP_MSG_LOG   0x04U

typedef enum
{
  ESP_INFO_BATTERY_SOC = 0x01,
  ESP_INFO_ERROR       = 0x02,
  ESP_INFO_PID_PARAMS  = 0x03
} EspLink_InfoId;

bool EspLink_SendInfo(EspLink_InfoId id, const void *data, uint8_t len);

infoID sets the byte length

├─────────────┼───────────────────────────────────────────────┼───────┤
│ BATTERY_SOC │ uint8_t percent                               │ 1     │
├─────────────┼───────────────────────────────────────────────┼───────┤
│ ERROR       │ uint16_t code, uint8_t severity               │ 3     │
├─────────────┼───────────────────────────────────────────────┼───────┤
│ PID_PARAMS  │ uint8_t loop_id, float kp, float ki, float kd │ 13    │
└─────────────┴───────────────────────────────────────────────┴──────

typedef struct __attribute__((packed))
{
  uint32_t sample_idx;   // IMU sample counter; time = idx / 833 Hz
  int16_t  gyro_x;       // raw LSB, same as LSM6DSOX_RawSample
  int16_t  gyro_y;
  int16_t  gyro_z;
  int16_t  accel_x;
  int16_t  accel_y;
  int16_t  accel_z;
} EspLink_ImuPayload;    // 16 bytes

typedef struct __attribute__((packed))
{
  uint8_t  motor_id;       // e.g. 0 = left wheel, 1 = right wheel
  int16_t  speed_rpm;
  int16_t  current_ma;
  int8_t   temp_c;
  uint8_t  status_flags;   // bitmask: enabled, fault, overtemp, ...
} EspLink_MotorInfo;       // 7 bytes, info_id = ESP_INFO_MOTOR

more frames to come

A new EspLink_Process() is used in the main loop to handle the incoming data from the ESP32, and to send out the outgoing data, such as IMU samples and status updates. It will also handle any commands received from the ESP32, such as changing motor speeds or PID gains.

void EspLink_Log(uint8_t level, const char *fmt, ...); can be called anywhere to log a message to the ESP32, which will be sent to the phone app for display
    use vsnprintf to format the message, and send it to the ESP32 over UART3

receive does not use a second DMA channel, instead it uses an interrupt HAL_UARTEx_ReceiveToIdle_IT

subscription system allows for certain data to be enabled, such as IMU streaming, or motor status updates, to reduce the amount of data sent over the link when not needed
    
Outputs
streams IMU data, and statuses, such as battery voltage, motor currents, etc. to the ESP32 for logging and visualization on the phone app

first we will start with IMU data
*/

#include "esp_link.h"

#include <string.h>

#define ESP_LINK_TX_BUF_SIZE        64U   /* Must stay a multiple of the 32-byte cache line. */
#define ESP_LINK_MAX_INFO_DATA      32U   /* Largest INFO data field; keeps a frame under one COBS block. */
#define ESP_LINK_FRAME_OVERHEAD     5U    /* type + seq + info_id + crc16. */
#define ESP_LINK_FRAME_DELIMITER    0x00U /* Ends every COBS-encoded frame. */

#define ESP_LINK_BATTERY_PERIOD_MS  1000U /* How often the battery SoC is sent. */
#define ESP_LINK_FAKE_BATTERY_SOC   78U   /* Placeholder percent until a real battery reading exists. */

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t s_tx_buf[ESP_LINK_TX_BUF_SIZE] __attribute__((aligned(32)));
static volatile uint8_t s_tx_busy = 0U;
static uint8_t s_seq = 0U;
static uint32_t s_dropped = 0U;
static uint32_t s_last_battery_ms = 0U;

static uint16_t esp_link_crc16(const uint8_t *data, uint16_t len);
static uint16_t esp_link_cobs_encode(const uint8_t *in, uint16_t len, uint8_t *out);

void EspLink_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
  s_tx_busy = 0U;
  s_seq = 0U;
  s_dropped = 0U;
  s_last_battery_ms = HAL_GetTick();
}

void EspLink_Process(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - s_last_battery_ms) >= ESP_LINK_BATTERY_PERIOD_MS)
  {
    uint8_t soc = ESP_LINK_FAKE_BATTERY_SOC;

    s_last_battery_ms = now;
    (void)EspLink_SendInfo(ESP_INFO_BATTERY_SOC, &soc, 1U);
  }
}

bool EspLink_SendInfo(EspLink_InfoId id, const void *data, uint8_t len)
{
  uint8_t raw[ESP_LINK_MAX_INFO_DATA + ESP_LINK_FRAME_OVERHEAD];
  uint16_t raw_len = 0U;
  uint16_t enc_len;
  uint16_t crc;

  if ((s_huart == NULL) || (data == NULL) || (len == 0U) || (len > ESP_LINK_MAX_INFO_DATA))
  {
    return false;
  }

  /* Single TX buffer for now: a frame that arrives mid-transfer is dropped. */
  if (s_tx_busy != 0U)
  {
    s_dropped++;
    return false;
  }

  raw[raw_len++] = ESP_MSG_INFO;
  raw[raw_len++] = s_seq;
  raw[raw_len++] = (uint8_t)id;
  memcpy(&raw[raw_len], data, len);
  raw_len += len;

  crc = esp_link_crc16(raw, raw_len);
  raw[raw_len++] = (uint8_t)(crc & 0xFFU);
  raw[raw_len++] = (uint8_t)(crc >> 8);

  enc_len = esp_link_cobs_encode(raw, raw_len, s_tx_buf);
  s_tx_buf[enc_len++] = ESP_LINK_FRAME_DELIMITER;

  /* The DMA reads RAM, not the D-Cache, so push the frame out to RAM first. */
  SCB_CleanDCache_by_Addr((uint32_t *)s_tx_buf, (int32_t)sizeof(s_tx_buf));

  s_tx_busy = 1U;
  if (HAL_UART_Transmit_DMA(s_huart, s_tx_buf, enc_len) != HAL_OK)
  {
    s_tx_busy = 0U;
    s_dropped++;
    return false;
  }

  /* seq only advances for frames that were actually sent. */
  s_seq++;
  return true;
}

uint32_t EspLink_GetDroppedCount(void)
{
  return s_dropped;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == s_huart)
  {
    s_tx_busy = 0U;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == s_huart)
  {
    s_tx_busy = 0U;
  }
}

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final XOR. */
static uint16_t esp_link_crc16(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0xFFFFU;

  for (uint16_t i = 0U; i < len; i++)
  {
    crc ^= (uint16_t)((uint16_t)data[i] << 8);
    for (uint8_t bit = 0U; bit < 8U; bit++)
    {
      if ((crc & 0x8000U) != 0U)
      {
        crc = (uint16_t)((crc << 1) ^ 0x1021U);
      }
      else
      {
        crc = (uint16_t)(crc << 1);
      }
    }
  }

  return crc;
}

/* COBS for inputs shorter than 254 bytes; writes len + 1 bytes and returns that count. */
static uint16_t esp_link_cobs_encode(const uint8_t *in, uint16_t len, uint8_t *out)
{
  uint16_t code_idx = 0U;
  uint16_t out_idx = 1U;
  uint8_t code = 1U;

  for (uint16_t i = 0U; i < len; i++)
  {
    if (in[i] == 0U)
    {
      out[code_idx] = code;
      code_idx = out_idx++;
      code = 1U;
    }
    else
    {
      out[out_idx++] = in[i];
      code++;
    }
  }
  out[code_idx] = code;

  return out_idx;
}
