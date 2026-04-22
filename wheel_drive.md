## Wheel Drive

Embedded C++ firmware for self-balancing and locomotion.
Reads IMU data, controls wheel motors, runs balancing algorithm.
Receives wheel position instructions from main system via USB.

### Decisions
- MCU: STM32N657ZOH (Cortex-M55 + Neural-ART NPU, LQFP144)
- Dev board: ST Nucleo (onboard ST-LINK, external Octo-SPI flash populated)
- Language: C
- Build: CubeMX `.ioc` + CMake + Ninja + `arm-none-eabi-gcc`
- Flash/debug: STM32CubeProgrammer (flash), OpenOCD + Cortex-Debug (debug)
- AI toolchain: STM32Cube AI Studio (STEDGEAI-CUBEAI, standalone) — X-CUBE-AI's successor; targets Neural-ART NPU via ST Edge AI Core

### TODO
- [ ] IMU sensor selection
- [ ] Wheel motor specs
- [ ] Balancing algorithm design
- [ ] USB communication protocol with main system
