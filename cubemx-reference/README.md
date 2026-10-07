# cubemx-reference

`cubemx-USART1/cubemx-USART.ioc` is a CubeMX project used for two things only:

- **Reserving resources** — see which pins, peripherals, and interrupts are already taken before adding something new.
- **Generating reference code** — init code to copy by hand into `Core/`.

It is not the source of truth. `Core/` and `CMakeLists.txt` are. Only `.ioc` files and this README are committed; all generated output here is gitignored.

## Adding a peripheral

1. Open the `.ioc` in STM32CubeMX. If asked about firmware versions, click **Continue**.
2. Enable the peripheral and assign its pins. Put it in the **FSBL** context, otherwise its code lands in a folder we don't use.
3. Check the Clock Configuration tab. The clock tree must match `SystemClock_Config()` in `Core/Src/main.c`, because CubeMX calculates baud rates, prescalers, and bit timings from it.
4. **Generate Code**. Output goes into this folder, never the repo root.
5. Copy what the peripheral needs from `cubemx-USART1/FSBL/Core/` into `Core/`:
   - `Src/main.c` — the handle, `MX_<periph>_Init()`, and its call
   - `Src/stm32n6xx_hal_msp.c` — `HAL_<periph>_MspInit/DeInit`
   - `Src/stm32n6xx_it.c` — any IRQ handler
   - `Inc/stm32n6xx_hal_conf.h` — `HAL_<periph>_MODULE_ENABLED`
6. Add the matching `stm32n6xx_hal_<periph>.c` driver sources to `CMakeLists.txt`.
7. Build with `cmake --build build`, then commit the `.ioc` with the `Core/` changes.

## Keep in mind

- If you change pins or peripherals by hand in `Core/`, update the `.ioc` too, or it will offer resources that are already in use.
- Do not copy generated project files, `Drivers/`, or the BSP calls (`BSP_LED_Init`, `BSP_COM_Init`); `Core/` uses the HAL directly.
- `OLD/` holds earlier reference projects and is not committed.
