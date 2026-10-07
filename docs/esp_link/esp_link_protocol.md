# ESP Link Protocol

Packet definition for the serial link between the STM32N6 (Cerebellum) and the ESP32. This is the document to write the ESP32 firmware from. For the reasoning behind the design and the STM32-side internals, see `esp_link.md`.

Each section is marked with what the STM32 firmware does today:

- **Implemented**: on the wire now.
- **Planned**: agreed format, not sent or accepted yet. Layouts may still change.

## 1. Physical Layer (Implemented)

| Setting | Value |
|---|---|
| Baud rate | 921600 |
| Format | 8 data bits, no parity, 1 stop bit |
| Flow control | None |
| Idle level | High |

Wiring:

| STM32N6 | Direction | ESP32 |
|---|---|---|
| `PD8` (`USART3_TX`) | to | UART RX pin |
| `PD9` (`USART3_RX`) | from | UART TX pin |
| `GND` | | `GND` |

- Use a UART other than the ESP32's UART0, which carries the ESP32 boot log and flashing traffic.
- **Check logic levels before connecting.** The ESP32 is 3.3 V. Confirm in the Nucleo manual (UM3417) that the board's I/O supply for port D is set to 3.3 V and not 1.8 V.
- The STM32 does not read its RX pin yet, so anything the ESP32 sends is ignored for now.

## 2. Frame Format (Implemented)

A frame before encoding:

```
[type:1][seq:1][payload:N][crc16:2]
```

On the wire, that frame is COBS-encoded and followed by one `0x00` delimiter byte:

```
[COBS-encoded frame][0x00]
```

| Field | Size | Meaning |
|---|---|---|
| `type` | 1 | Message type, see section 4 |
| `seq` | 1 | Frame counter, 0 to 255 then wraps. One counter for the whole link, shared by all types |
| `payload` | N | Depends on `type` |
| `crc16` | 2 | CRC over `type`, `seq` and `payload`, low byte first |

- **Multi-byte fields are little-endian.** The ESP32 is also little-endian, so packed structs can be copied directly.
- **CRC is CRC-16/CCITT-FALSE:** polynomial `0x1021`, initial value `0xFFFF`, no bit reflection, no final XOR. Check value: the ASCII string `123456789` gives `0x29B1`.
- **COBS** removes every `0x00` from the frame, so `0x00` appears on the wire only as the delimiter. The encoded frame is one byte longer than the unencoded one for frames under 254 bytes.
- **Sizes:** frames sent today are at most 39 bytes on the wire. Planned frames stay under 254 bytes unencoded. A 256-byte receive buffer covers everything.
- **There is no length field.** The delimiter ends the frame, and `type` (plus `info_id` for INFO frames) fixes the expected payload size.

## 3. Receiving a Frame

1. Collect bytes until a `0x00` arrives. Bytes before it are one encoded frame.
2. If the encoded frame is empty, ignore it. If it overflows the receive buffer, discard bytes until the next `0x00`.
3. COBS-decode it. If decoding fails, discard the frame.
4. The decoded frame must be at least 4 bytes (`type`, `seq`, CRC). If shorter, discard it.
5. Compute the CRC over everything except the last 2 bytes and compare it with those 2 bytes (low byte first). On mismatch, discard the frame and count a CRC error.
6. Check `seq` against the previous frame. A gap of `(uint8_t)(seq - last_seq - 1)` frames were lost on the wire. The STM32 only advances `seq` for frames it actually sends, so a gap never means the STM32 chose to drop something.
7. Dispatch on `type`. Discard a frame whose payload length does not match its type. Ignore unknown `type` and `info_id` values without treating them as errors, so new messages can be added later.

After power-up or any error, the receiver is back in sync at the next `0x00`. The first frame after connecting mid-stream will usually fail decoding or CRC; that is expected.

When the STM32 resets, `seq` restarts at 0. Treat a jump back to 0 as a restart, not as about 250 lost frames.

### Reference decoder

Both functions were tested against the vectors in section 6.

```c
/* Decodes one COBS frame (delimiter already removed). Returns the decoded length, or 0 if malformed. */
size_t esp_link_cobs_decode(const uint8_t *in, size_t len, uint8_t *out)
{
  size_t r = 0;
  size_t w = 0;

  while (r < len)
  {
    uint8_t code = in[r++];

    if ((code == 0) || ((r + code - 1) > len))
    {
      return 0;
    }
    for (uint8_t i = 1; i < code; i++)
    {
      out[w++] = in[r++];
    }
    if ((code != 0xFF) && (r < len))
    {
      out[w++] = 0;
    }
  }

  return w;
}

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, no final XOR. */
uint16_t esp_link_crc16(const uint8_t *data, size_t len)
{
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < len; i++)
  {
    crc ^= (uint16_t)(data[i] << 8);
    for (int bit = 0; bit < 8; bit++)
    {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
  }

  return crc;
}
```

The decoded output is never longer than the encoded input, so an output buffer the same size as the receive buffer is enough.

## 4. Message Types

| `type` | Name | Direction | State |
|---|---|---|---|
| `0x01` | `IMU` | STM32 to ESP32 | Implemented |
| `0x02` | `STATUS` | STM32 to ESP32 | Planned |
| `0x03` | `INFO` | STM32 to ESP32 | Implemented for `BATTERY_SOC` only |
| `0x04` | `LOG` | STM32 to ESP32 | Planned |
| `0x10` | `REQ` | ESP32 to STM32 | Planned |

### 4.1 INFO (`0x03`)

Payload:

```
[info_id:1][data:N]
```

| `info_id` | Name | Data | Bytes | State |
|---|---|---|---|---|
| `0x01` | `BATTERY_SOC` | `uint8_t percent` (0 to 100) | 1 | Implemented |
| `0x02` | `ERROR` | `uint16_t code`, `uint8_t severity` | 3 | Planned |
| `0x03` | `PID_PARAMS` | `uint8_t loop_id`, `float kp`, `float ki`, `float kd` | 13 | Planned |
| `0x04` | `MOTOR` | see below | 7 | Planned |

`BATTERY_SOC` is sent once a second. The value is currently hardcoded to 78; it is a placeholder, not a measurement.

`MOTOR` data, one frame per motor:

```c
typedef struct __attribute__((packed))
{
  uint8_t  motor_id;       /* e.g. 0 = left wheel, 1 = right wheel */
  int16_t  speed_rpm;
  int16_t  current_ma;
  int8_t   temp_c;
  uint8_t  status_flags;   /* bitmask: enabled, fault, overtemp, ... */
} EspLink_MotorInfo;       /* 7 bytes */
```

`float` is IEEE 754 single precision, 4 bytes.

The ESP32 can forward an INFO frame to the app without interpreting `data`.

### 4.2 IMU (`0x01`)

```c
typedef struct __attribute__((packed))
{
  uint32_t sample_idx;   /* IMU sample counter; time = idx / 833 Hz */
  int16_t  gyro_x;       /* raw counts */
  int16_t  gyro_y;
  int16_t  gyro_z;
  int16_t  accel_x;
  int16_t  accel_y;
  int16_t  accel_z;
} EspLink_ImuPayload;    /* 16 bytes */
```

Scale factors for the configured ranges (±2000 dps, ±4 g):

- Gyro: 70 mdps per count
- Accel: 0.122 mg per count

One frame per IMU sample, 22 bytes on the wire, about 18.3 kB/s at the full 833 Hz. The stream is always on; there is no way to turn it off until `REQ` exists.

`sample_idx` counts sensor data-ready events, not frames. It normally increases by 1. A larger step means the STM32 did not read or could not queue those samples; the sample time is still `sample_idx / 833 Hz`. A few samples are skipped once a second while the STM32 prints its debug report.

### 4.3 STATUS (`0x02`, Planned)

```c
typedef struct __attribute__((packed))
{
  uint8_t  proto_version;    /* bumped when any payload layout changes */
  uint8_t  state;            /* e.g. INIT, IDLE, BALANCING, FAULT */
  uint16_t fault_flags;      /* bitmask */
  uint32_t uptime_ms;
  uint16_t stream_dropped;   /* frames the STM32 refused for lack of buffer space */
  uint16_t fill_high_water;  /* peak TX buffer fill, bytes */
  uint16_t uart_errors;
} EspLink_StatusPayload;     /* 14 bytes */
```

Sent on change and once a second. The `state` and `fault_flags` values are not defined yet.

### 4.4 LOG (`0x04`, Planned)

Payload:

```
[level:1][text:N]
```

`text` is ASCII, up to about 64 characters, with no null terminator; its length is the payload length minus 1. Level values are not defined yet.

### 4.5 REQ (`0x10`, Planned, ESP32 to STM32)

Payload:

```
[cmd:1][info_id:1][period_ms:2]
```

| `cmd` | Meaning |
|---|---|
| `GET` | Send this `info_id` once |
| `SUBSCRIBE` | Send it every `period_ms`; a period of 0 unsubscribes |
| `UNSUBSCRIBE_ALL` | Stop everything optional |

- Numeric values for `cmd` are not assigned yet.
- Frames in this direction use the same format as section 2, with the ESP32 keeping its own `seq` counter.
- There is no acknowledgement frame; the requested data arriving is the confirmation.
- The ESP32 should send `UNSUBSCRIBE_ALL` when the Bluetooth connection drops. The STM32 will also expire a subscription not renewed within 5 seconds, so the app should re-send `SUBSCRIBE` every couple of seconds while it wants the data.

## 5. Current Behaviour Summary

What an ESP32 connected today will receive:

- One `IMU` frame per sample, about 833 per second, 22 bytes on the wire.
- One `INFO` / `BATTERY_SOC` frame per second, 8 bytes on the wire.
- Nothing else.
- `seq` starts at 0 after an STM32 reset and increases by 1 per frame, across both types.
- Frames arrive in bursts: several can be sent back to back in one block.

## 6. Test Vectors

`INFO` / `BATTERY_SOC` frames, battery SoC 78 (`0x4E`):

| `seq` | Unencoded frame | On the wire |
|---|---|---|
| 0 | `03 00 01 4E 27 85` | `02 03 05 01 4E 27 85 00` |
| 1 | `03 01 01 4E 17 B2` | `07 03 01 01 4E 17 B2 00` |
| 2 | `03 02 01 4E 47 EB` | `07 03 02 01 4E 47 EB 00` |

Reading the unencoded frame for `seq` 1: `03` is INFO, `01` is the sequence number, `01` is `BATTERY_SOC`, `4E` is 78%, and `17 B2` is the CRC `0xB217` low byte first.

The `seq` 0 frame looks different on the wire because its sequence byte is `0x00`, which COBS encodes away.

An `IMU` frame, `seq` 0, `sample_idx` 1, gyro (10, -20, 0), accel (-123, 45, 8197):

| | Bytes |
|---|---|
| Unencoded | `01 00 01 00 00 00 0A 00 EC FF 00 00 85 FF 2D 00 05 20 4D DC` |
| On the wire | `02 01 02 01 01 01 02 0A 03 EC FF 01 04 85 FF 2D 05 05 20 4D DC 00` |

## 7. Suggested ESP32 Diagnostics

Counting these makes link problems easy to tell apart:

| Counter | Meaning when it rises |
|---|---|
| Frames received OK | Link is healthy |
| Sequence gaps | Frames lost on the wire or in the ESP32's UART buffer |
| CRC errors | Corrupted bytes: noise, a baud mismatch, or a level problem |
| Decode or length errors | Corruption, or a protocol version mismatch |

## 8. Bluetooth Notes

- A BLE notification carries 20 bytes by default, and up to about 244 with a negotiated MTU. Every frame defined so far except long `LOG` lines fits in 20 bytes unencoded.
- The full-rate `IMU` stream is likely too much for a phone BLE connection and may need decimating on the ESP32.
