# ESP32 Link

Design notes for the serial link between the STM32N6 and the ESP32, which bridges robot data to a Bluetooth app.

**Status: IMU stream implemented.** `Core/Src/esp_link.c` streams every IMU sample as an IMU frame and sends one INFO frame, a hardcoded battery SoC of 78%, once a second, at 921600 baud through the ping-pong TX buffers. The IMU stream is always on until subscriptions exist. Everything else here is design: the other message types, receive, subscriptions and the diagnostics counters beyond a single `dropped` count. Once `esp_link_protocol.h` exists, it is the source of truth for message IDs and payload layouts, and this file explains the reasoning.

The packet definition for writing the ESP32 firmware is in `esp_link_protocol.md`.

## Hardware Configuration

Source: `cubemx-reference/CANandESP/CANandESP.ioc`, `Core/Src/stm32n6xx_hal_msp.c`

- UART: `USART3`, 8N1, no flow control
  - `PD8` = `USART3_TX`
  - `PD9` = `USART3_RX`
- TX DMA: `GPDMA1_Channel0`, request `GPDMA1_REQUEST_USART3_TX`, normal mode, byte width
- RX: no DMA channel (see Receive Path)
- Interrupts: `USART3_IRQn` and `GPDMA1_Channel0_IRQn`, both priority 5. The IMU EXTI is priority 0.

### Baud rate budget

A full-rate IMU stream is 22 bytes per frame at 833 Hz, about 18.3 kB/s. 115200 baud carries about 11.5 kB/s, so it does not fit.

- Minimum for the full IMU stream: 230400
- In use: 921600 (about 92 kB/s), set in `MX_USART3_UART_Init()`. From the 100 MHz PCLK the actual rate is 917431 baud, 0.45% low.

## Planned Module

- `Core/Inc/esp_link.h`, `Core/Src/esp_link.c` (listed explicitly in `CMakeLists.txt`)
- `esp_link_protocol.h`: message IDs and packed payload structs, shared with the ESP32 project

```c
void EspLink_Init(UART_HandleTypeDef *huart);
void EspLink_Process(void);   /* main loop, next to LSM6DSOX_AppProcess() */
bool EspLink_Send(uint8_t type, const void *payload, uint16_t len);   /* false = dropped */
bool EspLink_SendInfo(EspLink_InfoId id, const void *data, uint8_t len);
void EspLink_Log(uint8_t level, const char *fmt, ...);
```

Nothing in the module blocks. All sends are expected from the main loop; if a send is ever made from an ISR, the buffer append must be inside the critical section as well as the kick.

## TX Buffering

### D-Cache rule

D-Cache is enabled and no MPU region excludes DMA memory. The DMA reads physical RAM, not the cache, so every TX buffer must be:

- 32-byte aligned (`__attribute__((aligned(32)))`)
- a multiple of 32 bytes long
- cleaned with `SCB_CleanDCache_by_Addr()` before `HAL_UART_Transmit_DMA()`

Skipping this sends stale bytes intermittently.

### Ping-pong buffers

Two static buffers. The main loop appends whole frames to the fill buffer. When the DMA is idle, the buffers swap and the filled one is sent as one DMA block. Frames produced during a transfer batch into the next block.

- `HAL_UART_TxCpltCallback` clears the busy flag and starts the next block if the fill buffer is not empty.
- `HAL_UART_ErrorCallback` must also clear the busy flag, or one error stalls the link.
- The swap-and-start runs with BASEPRI raised to 5, so the IMU EXTI at priority 0 is never held off.
- Buffer size: 256 bytes each as a starting point. At 921600 baud that drains in under 3 ms.

A ring buffer was considered and not chosen: the load is steady, small, fixed-rate frames, so its better memory use does not justify the wrap handling. DMA circular mode is unsuitable for variable-length TX.

### Drop policy

- Droppable frames (IMU stream, logs) are refused once the fill buffer is more than 75% full.
- Status and fault frames may use the whole buffer, so the stream cannot crowd them out.
- A refused frame is counted, never retried.

## Frame Format

Before encoding:

```
[type:1][seq:1][payload:N][crc16:2]
```

Then COBS-encoded and terminated with a `0x00` delimiter. Wire cost is payload + 6 bytes for frames under 254 bytes.

- `seq`: one counter for the link, across all types. Assigned only when a frame is successfully appended, so a gap seen by the ESP32 means loss on the wire, not a drop at the source.
- CRC: CRC-16/CCITT-FALSE (poly `0x1021`, init `0xFFFF`, no reflection, no final XOR) over `type`, `seq` and payload, low byte first.
- No length field: the delimiter ends the frame, and the receiver rejects a frame whose decoded length does not match its type.
- Byte order: little-endian on both ends, so packed structs are copied directly. Each payload struct gets a `_Static_assert` on its size.

COBS was chosen over a sync byte plus length header because the receiver resyncs at the next `0x00` after any corruption, even with binary payloads.

## Message Types

| `type` | Name | Direction | Sent |
|---|---|---|---|
| `0x01` | `IMU` | STM32 to ESP32 | Stream |
| `0x02` | `STATUS` | STM32 to ESP32 | On change and once a second |
| `0x03` | `INFO` | STM32 to ESP32 | On request or subscription |
| `0x04` | `LOG` | STM32 to ESP32 | When called |
| `0x10` | `REQ` | ESP32 to STM32 | From the app |

### IMU (`0x01`)

```c
typedef struct __attribute__((packed))
{
  uint32_t sample_idx;   /* IMU sample counter; time = idx / 833 Hz */
  int16_t  gyro_x;       /* raw LSB, same as LSM6DSOX_RawSample */
  int16_t  gyro_y;
  int16_t  gyro_z;
  int16_t  accel_x;
  int16_t  accel_y;
  int16_t  accel_z;
} EspLink_ImuPayload;    /* 16 bytes, 22 on the wire */
```

Raw counts are sent; the receiver applies the scale factors (±2000 dps, ±4 g). `sample_idx` is separate from `seq` so the sample time survives decimation.

### STATUS (`0x02`)

```c
typedef struct __attribute__((packed))
{
  uint8_t  proto_version;    /* bump when any payload layout changes */
  uint8_t  state;            /* e.g. INIT, IDLE, BALANCING, FAULT */
  uint16_t fault_flags;      /* bitmask */
  uint32_t uptime_ms;
  uint16_t stream_dropped;
  uint16_t fill_high_water;
  uint16_t uart_errors;
} EspLink_StatusPayload;     /* 14 bytes, 20 on the wire */
```

Always sent, never subject to subscription. The periodic resend means a lost status frame corrects itself within a second.

### INFO (`0x03`)

Payload is `[info_id:1][data:N]`. The data is a variable-length run of bytes whose layout is set by `info_id`.

| `info_id` | Name | Data layout | Bytes |
|---|---|---|---|
| `0x01` | `BATTERY_SOC` | `uint8_t percent` | 1 |
| `0x02` | `ERROR` | `uint16_t code`, `uint8_t severity` | 3 |
| `0x03` | `PID_PARAMS` | `uint8_t loop_id`, `float kp`, `float ki`, `float kd` | 13 |
| `0x04` | `MOTOR` | `EspLink_MotorInfo` | 7 |

```c
typedef struct __attribute__((packed))
{
  uint8_t  motor_id;       /* e.g. 0 = left wheel, 1 = right wheel */
  int16_t  speed_rpm;
  int16_t  current_ma;
  int8_t   temp_c;
  uint8_t  status_flags;   /* bitmask: enabled, fault, overtemp, ... */
} EspLink_MotorInfo;       /* 7 bytes, one frame per motor */
```

The ESP32 forwards INFO frames to the app without interpreting them, so a new `info_id` needs changes on the STM32 and in the app only.

### LOG (`0x04`)

Payload is `[level:1][text:N]`, with no null terminator. `EspLink_Log` formats with `vsnprintf` into a local buffer capped at about 64 characters and truncates beyond that. Logs are droppable.

### REQ (`0x10`)

Payload is `[cmd:1][info_id:1][period_ms:2]`.

| `cmd` | Meaning |
|---|---|
| `GET` | Send this `info_id` once |
| `SUBSCRIBE` | Send it every `period_ms`; a period of 0 unsubscribes |
| `UNSUBSCRIBE_ALL` | Stop everything optional |

There is no acknowledgement frame; the requested data arriving is the confirmation.

## Subscriptions

INFO items are sent only when the app asks. The STM32 holds a table:

```c
typedef struct
{
  uint8_t  info_id;
  uint8_t  (*read)(uint8_t *out);   /* fills data, returns its length */
  uint16_t period_ms;               /* 0 = not subscribed */
  uint32_t next_due_ms;
} EspLink_InfoItem;
```

`EspLink_Process()` decodes received requests and updates the table, then sends every subscribed item that is due. An unsubscribed item's `read` function is never called.

When the app goes away:

- The ESP32 sends `UNSUBSCRIBE_ALL` when the Bluetooth connection drops.
- The STM32 expires a subscription not renewed within 5 seconds. The app re-sends `SUBSCRIBE` every couple of seconds while a screen is open.

## Receive Path

Requests are a few bytes at a low rate, so receive is interrupt-driven with `HAL_UARTEx_ReceiveToIdle_IT`. This needs no second DMA channel, no CubeMX change, and no D-Cache handling. The callback copies received bytes into a small ring buffer; COBS decoding, CRC checking and dispatch all happen in `EspLink_Process()`.

## Diagnostics

Counters kept by the module:

- `dropped`, per message type: frames refused for lack of buffer space
- `fill_high_water`: largest fill level seen, which shows remaining margin before any drop occurs
- `uart_errors`: incremented in `HAL_UART_ErrorCallback`

They are printed with the once-a-second report on the debug UART and carried in the STATUS frame. The ESP32 counts sequence gaps and CRC failures.

| Symptom | Meaning |
|---|---|
| `dropped` rising on the STM32 | Producing faster than the link drains |
| Sequence gaps on the ESP32, `dropped` still 0 | Loss on the wire or in the ESP32 receive buffer |
| CRC failures | Corrupted bytes |

### Forced-failure tests

Run each once to prove the detection works before trusting zeros:

1. Overflow: stream every IMU sample at 115200 baud. `dropped` climbs and `fill_high_water` reaches the limit.
2. Wire loss: pull the TX wire briefly while streaming. The ESP32 reports gaps and CRC failures; `dropped` stays 0.
3. Status protection: send STATUS frames during the overflow test. Their drop counter stays 0.

## Open Items

- Final baud rate (921600 in use, to be confirmed against the ESP32).
- Motor fields and units, to match what the motor controller reports.
- BLE limits: a notification carries 20 bytes by default (up to about 244 with a negotiated MTU), so log lines may need splitting on the ESP32. The full 833 Hz IMU stream is likely too much for a phone BLE connection and may need decimating on the ESP32 or keeping off Bluetooth.
- Whether any message needs a delivery guarantee beyond the periodic STATUS resend, which would require an ack frame.
