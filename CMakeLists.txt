cmake_minimum_required(VERSION 3.22)

project(cerebellum LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/cmake/STM32N657X0HXQ_AXISRAM2_fsbl.ld)

file(GLOB APPLICATION_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/Application/Src/*.c
)

file(GLOB MODEL_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/Models/*.c
)

add_executable(${PROJECT_NAME}
  Core/Src/main.c
  Core/Src/lsm6dsox_imu.c
  Core/Src/stm32n6xx_hal_msp.c
  Core/Src/stm32n6xx_it.c
  Core/Src/system_stm32n6xx_fsbl.c
  Core/Src/syscalls.c
  Core/Src/sysmem.c
  Core/Startup/startup_stm32n657x0hxq_fsbl.s
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_cortex.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_dma.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_dma_ex.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_exti.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_gpio.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pwr.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pwr_ex.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rcc.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rcc_ex.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_spi.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_spi_ex.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_uart.c
  Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_uart_ex.c
  ${APPLICATION_SOURCES}
  ${MODEL_SOURCES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

target_include_directories(${PROJECT_NAME} PRIVATE
  Core/Inc
  Application/Inc
  Models
  Drivers/CMSIS/Include
  Drivers/CMSIS/Device/ST/STM32N6xx/Include
  Drivers/STM32N6xx_HAL_Driver/Inc
  Drivers/STM32N6xx_HAL_Driver/Inc/Legacy
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  USE_HAL_DRIVER
  STM32N657xx
  $<$<CONFIG:Debug>:DEBUG>
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -mcmse
  $<$<CONFIG:Debug>:-Og>
  $<$<CONFIG:Debug>:-g3>
  $<$<CONFIG:Release>:-Os>
)

target_link_options(${PROJECT_NAME} PRIVATE
  -mcmse
  -T${LINKER_SCRIPT}
  -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
  -Wl,--print-memory-usage
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
)
