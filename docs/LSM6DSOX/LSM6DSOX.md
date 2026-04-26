# LSM6DSOX

LSM6DSOX IMU notes and quick index for this repository.

## Local Datasheet

- `LSM6DSOX.pdf` (local copy in this folder)

## Reference Code Index

- ST Standard C reference repository: `reference/lsm6dsox_STdC`
- Example programs: `reference/lsm6dsox_STdC/examples`
- Driver source used as the baseline reference: `reference/lsm6dsox_STdC/driver/lsm6dsox-pid`

## Selected Baseline Example

- We will reference: `reference/lsm6dsox_STdC/examples/lsm6dsox_read_data_drdy.c`
- Reason: this example is data-ready interrupt driven (`drdy`), which is a good base for low-latency, deterministic sampling in a self-balancing robot control loop.

## Current Hardware Configuration (CubeMX)

Source: `cubemx-reference/cubemx-SPI5/cubemx-SPI5.ioc`

- IMU bus: `SPI5` (full-duplex master)
- SPI pins:
  - `PE15` = `SPI5_SCK`
  - `PG1` = `SPI5_MISO`
  - `PG2` = `SPI5_MOSI`
- Chip select:
  - `PA3` = `IMU_CS` as software-controlled GPIO output
  - Default state set high (inactive)
- Interrupt pin:
  - `PD12` = `IMU_INT` on `GPIO_EXTI12`
  - `EXTI12_IRQn` enabled in NVIC
- SPI runtime settings:
  - `Motorola`, `8-bit`, `MSB first`
  - `Mode 3` (`CPOL=High`, `CPHA=2Edge`)
  - Prescaler `/32` (currently `3.125 Mbit/s`)

## Integration Notes

- Keep upstream ST code in `reference/` as a reference snapshot.
- Implement project-owned IMU interface in:
  - `Core/Inc`
  - `Core/Src`
- During bring-up, first verify `WHO_AM_I` and stable `drdy` interrupts, then increase SPI speed only if needed.
