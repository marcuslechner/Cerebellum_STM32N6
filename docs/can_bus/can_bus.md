# CAN Bus

Wiring and configuration notes for the CAN bus between the STM32N6 and the GIM6010-8 motors.

**Status: bus started, receive and transmit.** `HDW_CAN_Init()` (`Core/Src/HDW_CAN.c`) sets the receive filter and starts FDCAN3; `APP_MOTOR.c` listens for the heartbeat of one motor. Receive is polled from RX FIFO 0 in the main loop, so no FDCAN interrupt source is enabled. Transmit queues frames in the TX FIFO. `APP_MOTOR_SetPosition()` sets a target in output shaft turns; `APP_MOTOR.c` puts the motor in position control with a trapezoidal trajectory and sends the target once (rotor turns, 8:1 ratio). Errors the motor reports are cleared with `Clear_Errors` and it is enabled again. With `MOTOR_SWEEP_TEST` set in `app_config.h` the target alternates between `APP_MOTOR_SWEEP_MIN_OUTPUT_TURNS` and `APP_MOTOR_SWEEP_MAX_OUTPUT_TURNS` at `APP_MOTOR_TRAJ_OUTPUT_RPM`. With `MOTOR_VEL_TEST` set in `app_config.h`, `APP_MOTOR.c` instead puts the motor in ramped velocity control and runs it at `APP_MOTOR_TEST_OUTPUT_REV_S` (output shaft rev/s, converted to rotor turn/s with the 8:1 ratio). The transceiver wiring and termination below are the plan; they have not been built or tested.

## Why classic CAN, not CAN FD

The GIM6010-8 only speaks classic CAN (manual section 4.1): standard frames, 11-bit ID, 8 data bytes, 1 Mbit/s maximum. A classic-only node treats every CAN FD frame as a bus error and destroys it, so FD frames cannot be used on a bus shared with these motors. The FDCAN peripheral and the MCP2562FD transceiver both run classic CAN without restriction.

## Hardware Configuration

Source: `cubemx-reference/CANandESP/CANandESP.ioc`, `Core/Src/main.c`, `Core/Src/stm32n6xx_hal_msp.c`

- Peripheral: `FDCAN3`, classic frame format, normal mode
  - `PE11` = `FDCAN3_TX` (Arduino D7, CN13 pin 8; also CN15 pin 23)
  - `PE12` = `FDCAN3_RX` (CN15 pin 1, morpho header only)
- Kernel clock: PCLK1, 100 MHz
- Bit timing: prescaler 5, Seg1 15, Seg2 4, sync jump width 4. 50 ns quantum, 20 quanta per bit, 1 Mbit/s, sample point 80%
- Auto retransmission: on
- Message memory: 1 standard filter, 16-frame RX FIFO 0, 8-frame TX FIFO, 8-byte elements
- Interrupt: `FDCAN3_IT0_IRQn`, priority 0

### Motor settings to match

The motors ship at 500 kbit/s and must be set to the bus rate with odrivetool:

- `odrv0.can.config.baud_rate` = 1000000
- `odrv0.axis0.config.can.node_id` = unique per motor
- `odrv0.can.config.enable_r120` = termination resistor, see Termination

For 500 kbit/s on the STM32 side instead, change the prescaler to 10 and leave the rest.

## Transceiver: MCP2562FD

Datasheet: `docs/MCP2562.pdf` (gitignored)

- Needs a 5 V supply on VDD (4.5 to 5.5 V). It does not run from 3.3 V.
- VIO (1.8 to 5.5 V) sets the logic level, so it connects to the STM32's 3.3 V pins without a level shifter.
- STBY has an internal pull-up. Left unconnected, the chip stays in standby and does not transmit.

| MCP2562FD pin | Connect to |
|---|---|
| 1 TXD | `PE11` (FDCAN3 TX) |
| 2 VSS | Ground |
| 3 VDD | 5 V from the Nucleo |
| 4 RXD | `PE12` (FDCAN3 RX) |
| 5 VIO | 3.3 V from the Nucleo |
| 6 CANL | Bus CANL |
| 7 CANH | Bus CANH |
| 8 STBY | Ground (normal mode) |

Fit a 100 nF decoupling capacitor close to each of VDD and VIO. The motors and the Nucleo need a common ground.

## Termination

A CAN bus is a single line with one 120 Ω resistor across CANH/CANL at each physical end: two resistors in total, never one and never three. With only one, signals reflect off the open end, which at 1 Mbit/s causes intermittent errors and retransmissions even if a short bench cable appears to work.

Which resistors to use depends on where the STM32 sits on the line:

| Layout | Termination |
|---|---|
| STM32 at one end, motors chained after it | 120 Ω next to the transceiver, and the built-in resistor on in the last motor only |
| STM32 in the middle, a motor out each side | Built-in resistor on in both end motors, no resistor at the STM32 |

- Any motor that is not at an end of the line has its resistor off.
- Keep the branch from the main line to each device short, a few centimetres where possible.

### Checking it

With everything powered off, measure resistance between CANH and CANL:

- About 60 Ω: two terminators, correct
- About 120 Ω: only one
- About 40 Ω: three
