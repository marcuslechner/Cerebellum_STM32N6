# Reference Documents

ST reference documentation for STM32N657 / Nucleo-N657 / STM32CubeN6. PDFs are `.gitignore`d — download your own copies into this directory to follow along.

Search [st.com](https://www.st.com) by the document number (e.g. `RM0486`) to find the latest revision. All are free downloads.

## Core MCU

- **RM0486** — STM32N647/657xx Arm-based 32-bit MCUs reference manual. Primary register / peripheral reference.
- **PM0273** — STM32 Cortex-M55 MCUs programming manual. Core, MVE/Helium, debug.

## Board

- **UM3417** — STM32N6 Nucleo-144 board (MB1940) user manual. Pinout, jumpers, ST-LINK/V3E.
- **NUCLEO-N657X0-Q product flyer** (`nucleo-n657x0-q-1.pdf`) — board feature summary.

## STM32CubeN6 MCU package

- **UM3249** — Getting started with STM32CubeN6 for STM32N6 series.
- **UM3425** — Description of STM32N6 HAL and low-layer drivers. Primary HAL API reference.
- **UM2298** — STM32Cube BSP drivers development guidelines.
- **UM1722** — Developing applications on STM32Cube with RTOS.
- **AN6197** — STM32Cube MCU package examples for STM32N6 MCUs.
- **STM32CubeN6 package overview** (`stm32cuben6.pdf`).

## Cleanup notes for the existing local dump

Two files in the current `docs/` tree are duplicates — safe to delete one copy each:

- `docs/STM32CubeN6 MCU Package/um1722-...-1.pdf` vs `docs/STM32CubeN6 MCU Package/um1722-...pdf`
- `docs/um3249-...-1.pdf` vs `docs/STM32CubeN6 MCU Package/um3249-...-1.pdf`
