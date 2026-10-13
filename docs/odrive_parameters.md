# GIM6010-8 driver parameters

Every property the GIM6010-8 driver exposes over the ODrive ASCII protocol, with the value read from the motor on 2026-10-13.

- **Device:** hardware 3.7 variant 1, firmware 0.5.11, serial 124562866
- **Property count:** 434, of which 4 are vendor additions not in stock ODrive
- **Firmware match:** all 430 single-axis properties of stock ODrive 0.5.6 are present; `axis1` is not
- **Values:** a snapshot of one motor. Telemetry values (position, voltage, temperatures) change continuously

## How this was built

Candidate names came from the official ODrive interface definitions (0.5.1, 0.5.2, 0.5.4, 0.5.6) and the GIM6010-8 manual. Each one was read from the live driver, and only names the driver answered are listed.

The ASCII protocol cannot list properties, so a vendor addition that the manual does not mention would not be found this way. Types and descriptions are from stock 0.5.6; the vendor firmware may behave differently where noted.

## Sources

- Official API reference for 0.5.6: <https://docs.odriverobotics.com/v/0.5.6/fibre_types/com_odriverobotics_ODrive.html>
- Machine-readable definitions: <https://raw.githubusercontent.com/odriverobotics/ODrive/fw-v0.5.6/Firmware/odrive-interface.yaml>
- ASCII protocol: <https://docs.odriverobotics.com/v/0.5.6/ascii-protocol.html>
- Vendor manual: `Legs/Cerebellum_STM32N6/docs/GIM6010-8_Manual_rev2.2_EN.pdf`

## Reading and writing

| ASCII command | Effect |
|---|---|
| `r <property>` | Read a property, e.g. `r can.config.baud_rate` |
| `w <property> <value>` | Write a property, e.g. `w axis0.config.can.node_id 1` |
| `ss` | Save configuration to flash. The driver reboots and the serial port re-enumerates |
| `se` | Erase configuration and restore defaults |
| `sr` | Reboot |
| `sc` | Clear errors |

In `odrivetool` the same properties are `odrv0.<property>`. Writes are lost at power-off unless saved.

## System

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `brake_resistor_armed` | bool | read-only | 0 |  |
| `brake_resistor_current` | float32 | read-only | 0.000000 | Commanded brake resistor current |
| `brake_resistor_saturated` | bool | read-only | 0 |  |
| `error` | flags | read/write | 0 (none) |  |
| `fw_version_major` | uint8 | read-only | 0 |  |
| `fw_version_minor` | uint8 | read-only | 5 |  |
| `fw_version_revision` | uint8 | read-only | 11 |  |
| `fw_version_unreleased` | uint8 | read-only | 1 | 0 for official releases, 1 otherwise |
| `hw_version_major` | uint8 | read-only | 3 |  |
| `hw_version_minor` | uint8 | read-only | 7 | The manual says current 6010-8 boards are minor version 8; this one reports 7. |
| `hw_version_variant` | uint8 | read-only | 1 |  |
| `ibus` | float32 | read-only | 0.000000 | Current on the DC bus as calculated by the ODrive. Unit: A. |
| `ibus_report_filter_k` | float32 | read/write | 1.000000 | Filter gain for the reported `ibus`. Set to a value below 1.0 to get a smoother line when plotting `ibus`. Set to 1.0 to disable. This filter is only applied to the reported value and not for internal calculations. |
| `misconfigured` | bool | read-only | 1 | If this property is true, something is bad in the configuration. The ODrive can still be used in this state but the user should investigate which setting is problematic. |
| `n_evt_control_loop` | uint32 | read-only | 4886802 | Number of control loop iterations since startup (modulo 2^32) |
| `n_evt_sampling` | uint32 | read-only | 4886885 | Number of input sampling events since startup (modulo 2^32) |
| `otp_valid` | bool | read-only | 1 |  |
| `serial_number` | uint64 | read-only | 124562866 |  |
| `task_timers_armed` | bool | read/write | 0 | Set by a profiling application to trigger sampling of a single control iteration. Cleared by the device as soon as the sampling is complete. |
| `test_property` | uint32 | read/write | 0 |  |
| `user_config_loaded` | uint32 | read-only | 15756 |  |
| `vbus_voltage` | float32 | read-only | 23.552605 | Voltage on the DC bus as measured by the ODrive. Unit: V. |

## System configuration (`config.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `config.brake_resistance` | float32 | read/write | 2.000000 | Value of the brake resistor connected to the ODrive. Unit: ohm. |
| `config.dc_bus_overvoltage_ramp_end` | float32 | read/write | 64.200005 | See `enable_dc_bus_overvoltage_ramp`. |
| `config.dc_bus_overvoltage_ramp_start` | float32 | read/write | 64.200005 | See `enable_dc_bus_overvoltage_ramp`. |
| `config.dc_bus_overvoltage_trip_level` | float32 | read/write | 64.200005 | Maximum voltage above which the motor stops operating. Unit: V. |
| `config.dc_bus_undervoltage_trip_level` | float32 | read/write | 15.000000 | Minimum voltage below which the motor stops operating. Unit: V. |
| `config.dc_max_negative_current` | float32 | read/write | -10.00000 | Max current the power supply can sink. Unit: A. |
| `config.dc_max_positive_current` | float32 | read/write | 20.000000 | Max current the power supply can source. Unit: A. |
| `config.enable_brake_resistor` | bool | read/write | 0 | Enable/disable the use of a brake resistor. |
| `config.enable_can_a` | bool | read/write | 0 | Enables CAN. Changing this setting requires a reboot. Vendor manual: on hardware ≤ 3.7, USB and CAN cannot be used together. Setting this true and saving moves the driver to CAN only; the way back is the CAN `Disable_Can` message (0x01E). |
| `config.enable_dc_bus_overvoltage_ramp` | bool | read/write | 0 | Enables the DC bus overvoltage ramp feature. |
| `config.enable_i2c_a` | bool | read/write | 1 | Enables I2C. The I2C pins on ODrive v3.x are in conflict with CAN. This setting has no effect if `enable_can_a` is also true. This setting has no effect on ODrive v3.2 or earlier. Changing this setting requires a reboot. |
| `config.enable_uart_a` | bool | read/write | 1 | Enables/disables UART_A. |
| `config.enable_uart_b` | bool | read/write | 0 | Enables/disables UART_B. |
| `config.enable_uart_c` | bool | read/write | 0 | Not supported on ODrive v3.x. |
| `config.error_gpio_pin` | uint32 | read/write | 3 |  |
| `config.gpio10_mode` | GpioMode | read/write | 3 (ANALOG_IN) | Mode of GPIO10 (changes take effect after reboot) |
| `config.gpio11_mode` | GpioMode | read/write | 15 (STATUS) | Mode of GPIO11 (changes take effect after reboot) |
| `config.gpio12_mode` | GpioMode | read/write | 15 (STATUS) | Mode of GPIO12 (changes take effect after reboot) |
| `config.gpio13_mode` | GpioMode | read/write | 8 (I2C_A) | Mode of GPIO13 (changes take effect after reboot) |
| `config.gpio14_mode` | GpioMode | read/write | 8 (I2C_A) | Mode of GPIO14 (changes take effect after reboot) |
| `config.gpio15_mode` | GpioMode | read/write | 7 (CAN_A) | Mode of GPIO15 (changes take effect after reboot) |
| `config.gpio16_mode` | GpioMode | read/write | 7 (CAN_A) | Mode of GPIO16 (changes take effect after reboot) |
| `config.gpio1_mode` | GpioMode | read/write | 1 (DIGITAL_PULL_UP) | Mode of GPIO1 (changes take effect after reboot) |
| `config.gpio1_pwm_mapping.endpoint` | endpoint_ref | read/write | not readable over ASCII |  |
| `config.gpio1_pwm_mapping.max` | float32 | read/write | 0.000000 |  |
| `config.gpio1_pwm_mapping.min` | float32 | read/write | 0.000000 |  |
| `config.gpio2_mode` | GpioMode | read/write | 1 (DIGITAL_PULL_UP) | Mode of GPIO2 (changes take effect after reboot) |
| `config.gpio2_pwm_mapping.endpoint` | endpoint_ref | read/write | not readable over ASCII |  |
| `config.gpio2_pwm_mapping.max` | float32 | read/write | 0.000000 |  |
| `config.gpio2_pwm_mapping.min` | float32 | read/write | 0.000000 |  |
| `config.gpio3_analog_mapping.endpoint` | endpoint_ref | read/write | not readable over ASCII |  |
| `config.gpio3_analog_mapping.max` | float32 | read/write | 0.000000 |  |
| `config.gpio3_analog_mapping.min` | float32 | read/write | 0.000000 |  |
| `config.gpio3_mode` | GpioMode | read/write | 2 (DIGITAL_PULL_DOWN) | Mode of GPIO3 (changes take effect after reboot) |
| `config.gpio3_pwm_mapping.endpoint` | endpoint_ref | read/write | not readable over ASCII |  |
| `config.gpio3_pwm_mapping.max` | float32 | read/write | 0.000000 |  |
| `config.gpio3_pwm_mapping.min` | float32 | read/write | 0.000000 |  |
| `config.gpio4_analog_mapping.endpoint` | endpoint_ref | read/write | not readable over ASCII |  |
| `config.gpio4_analog_mapping.max` | float32 | read/write | 0.000000 |  |
| `config.gpio4_analog_mapping.min` | float32 | read/write | 0.000000 |  |
| `config.gpio4_mode` | GpioMode | read/write | 3 (ANALOG_IN) | Mode of GPIO4 (changes take effect after reboot) |
| `config.gpio4_pwm_mapping.endpoint` | endpoint_ref | read/write | not readable over ASCII |  |
| `config.gpio4_pwm_mapping.max` | float32 | read/write | 0.000000 |  |
| `config.gpio4_pwm_mapping.min` | float32 | read/write | 0.000000 |  |
| `config.gpio5_mode` | GpioMode | read/write | 15 (STATUS) | Mode of GPIO5 (changes take effect after reboot). The driver uses GPIO5 for the CAN termination switch (`can.config.r120_gpio_num`), so the stock name STATUS may not apply. |
| `config.gpio6_mode` | GpioMode | read/write | 10 (PWM) | Mode of GPIO6 (changes take effect after reboot) |
| `config.gpio7_mode` | GpioMode | read/write | 0 (DIGITAL) | Mode of GPIO7 (changes take effect after reboot) |
| `config.gpio8_mode` | GpioMode | read/write | 3 (ANALOG_IN) | Mode of GPIO8 (changes take effect after reboot) |
| `config.gpio9_mode` | GpioMode | read/write | 3 (ANALOG_IN) | Mode of GPIO9 (changes take effect after reboot) |
| `config.max_regen_current` | float32 | read/write | 0.000000 | The bus current allowed to flow back to the power supply before the brake resistor module will start shunting current. Unit: Amps. |
| `config.uart0_protocol` | StreamProtocolType | read/write | 3 (AsciiAndStdout) |  |
| `config.uart1_protocol` | StreamProtocolType | read/write | 3 (AsciiAndStdout) |  |
| `config.uart2_protocol` | StreamProtocolType | read/write | 3 (AsciiAndStdout) |  |
| `config.uart_a_baudrate` | uint32 | read/write | 115200 | Defines the baudrate used on the UART interface. Unit: baud/s. |
| `config.uart_b_baudrate` | uint32 | read/write | 115200 | Defines the baudrate used on the UART interface. Unit: baud/s. |
| `config.uart_c_baudrate` | uint32 | read/write | 115200 | Not supported on ODrive v3.x. |
| `config.usb_cdc_protocol` | StreamProtocolType | read/write | 3 (AsciiAndStdout) | The protocol that's being run on the device's virtual COM port on USB. |

## CAN (`can.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `can.config.baud_rate` | uint32 | read/write | 1000000 |  |
| `can.config.enable_r120` | bool | read/write | 1 | Switches the on-board 120 Ω CAN termination resistor. |
| `can.config.protocol` | Protocol | read/write | 1 (SIMPLE) |  |
| `can.config.r120_gpio_num` | uint | read/write | 5 | GPIO number that drives the termination resistor switch. The manual uses 5. |
| `can.error` | flags | read/write | 0 (none) |  |

## System statistics (`system_stats.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `system_stats.i2c.addr` | uint8 | read-only | 104 |  |
| `system_stats.i2c.addr_match_cnt` | uint32 | read-only | 0 |  |
| `system_stats.i2c.error_cnt` | uint32 | read-only | 0 |  |
| `system_stats.i2c.rx_cnt` | uint32 | read-only | 0 |  |
| `system_stats.max_stack_usage_analog` | uint32 | read-only | 0 |  |
| `system_stats.max_stack_usage_axis` | uint32 | read-only | 0 |  |
| `system_stats.max_stack_usage_can` | uint32 | read-only | 0 |  |
| `system_stats.max_stack_usage_startup` | uint32 | read-only | 0 |  |
| `system_stats.max_stack_usage_uart` | uint32 | read-only | 0 |  |
| `system_stats.max_stack_usage_usb` | uint32 | read-only | 0 |  |
| `system_stats.min_heap_space` | uint32 | read-only | 0 |  |
| `system_stats.prio_analog` | int32 | read-only | 0 |  |
| `system_stats.prio_axis` | int32 | read-only | 0 |  |
| `system_stats.prio_can` | int32 | read-only | 0 |  |
| `system_stats.prio_startup` | int32 | read-only | 0 |  |
| `system_stats.prio_uart` | int32 | read-only | 0 |  |
| `system_stats.prio_usb` | int32 | read-only | 0 |  |
| `system_stats.stack_size_analog` | uint32 | read-only | 0 |  |
| `system_stats.stack_size_axis` | uint32 | read-only | 0 |  |
| `system_stats.stack_size_can` | uint32 | read-only | 0 |  |
| `system_stats.stack_size_startup` | uint32 | read-only | 0 |  |
| `system_stats.stack_size_uart` | uint32 | read-only | 0 |  |
| `system_stats.stack_size_usb` | uint32 | read-only | 0 |  |
| `system_stats.uptime` | uint32 | read-only | 0 |  |
| `system_stats.usb.rx_cnt` | uint32 | read-only | 1120 |  |
| `system_stats.usb.tx_cnt` | uint32 | read-only | 0 |  |
| `system_stats.usb.tx_overrun_cnt` | uint32 | read-only | 0 |  |

## Axis (`axis0.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.current_state` | AxisState | read-only | 1 (IDLE) | The current state of the axis |
| `axis0.error` | flags | read/write | 0 (none) |  |
| `axis0.is_homed` | bool | read/write | 0 | Whether or not the axis has been successfully homed. |
| `axis0.last_drv_fault` | uint32 | read-only | 0 |  |
| `axis0.requested_state` | AxisState | read/write | 0 (UNDEFINED) | The user's commanded axis state |
| `axis0.step_dir_active` | bool | read-only | 0 |  |
| `axis0.steps` | int64 | read-only | 0 | The current commanded position, in steps, while in `step_dir` mode |

## Axis configuration (`axis0.config.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.config.calibration_lockin.accel` | float32 | read/write | 20.000000 |  |
| `axis0.config.calibration_lockin.current` | float32 | read/write | 10.000000 |  |
| `axis0.config.calibration_lockin.ramp_distance` | float32 | read/write | 3.141593 |  |
| `axis0.config.calibration_lockin.ramp_time` | float32 | read/write | 0.400000 |  |
| `axis0.config.calibration_lockin.vel` | float32 | read/write | 40.000000 |  |
| `axis0.config.can.bus_vi_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.controller_error_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.encoder_count_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.encoder_error_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.encoder_rate_ms` | uint32 | read/write | 10 |  |
| `axis0.config.can.heartbeat_rate_ms` | uint32 | read/write | 100 |  |
| `axis0.config.can.iq_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.is_extended` | bool | read/write | 0 |  |
| `axis0.config.can.motor_error_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.node_id` | uint32 | read/write | 1 |  |
| `axis0.config.can.sensorless_error_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.can.sensorless_rate_ms` | uint32 | read/write | 0 |  |
| `axis0.config.dir_gpio_pin` | uint16 | read/write | 0 |  |
| `axis0.config.enable_sensorless_mode` | bool | read/write | 0 |  |
| `axis0.config.enable_step_dir` | bool | read/write | 0 | Enable step/dir input after calibration. Make sure to set the corresponding GPIO's mode to `GPIO_MODE_DIGITAL`. |
| `axis0.config.enable_watchdog` | bool | read/write | 0 |  |
| `axis0.config.general_lockin.accel` | float32 | read/write | 20.000000 | Unit: rad/s^2. |
| `axis0.config.general_lockin.current` | float32 | read/write | 10.000000 | Unit: A. |
| `axis0.config.general_lockin.finish_distance` | float32 | read/write | 100.00000 | Unit: rad. |
| `axis0.config.general_lockin.finish_on_distance` | bool | read/write | 0 |  |
| `axis0.config.general_lockin.finish_on_enc_idx` | bool | read/write | 0 |  |
| `axis0.config.general_lockin.finish_on_vel` | bool | read/write | 0 |  |
| `axis0.config.general_lockin.ramp_distance` | float32 | read/write | 3.141593 | Unit: rad. |
| `axis0.config.general_lockin.ramp_time` | float32 | read/write | 0.400000 | Unit: s. |
| `axis0.config.general_lockin.vel` | float32 | read/write | 40.000000 | Unit: rad/s. |
| `axis0.config.sensorless_ramp.accel` | float32 | read/write | 200.00000 | Unit: rad/s^2. |
| `axis0.config.sensorless_ramp.current` | float32 | read/write | 10.000000 | Unit: A. |
| `axis0.config.sensorless_ramp.finish_distance` | float32 | read/write | 100.00000 | Unit: rad. |
| `axis0.config.sensorless_ramp.finish_on_distance` | bool | read/write | 0 |  |
| `axis0.config.sensorless_ramp.finish_on_enc_idx` | bool | read/write | 0 |  |
| `axis0.config.sensorless_ramp.finish_on_vel` | bool | read/write | 1 |  |
| `axis0.config.sensorless_ramp.ramp_distance` | float32 | read/write | 3.141593 | Unit: rad. |
| `axis0.config.sensorless_ramp.ramp_time` | float32 | read/write | 0.400000 | Unit: s. |
| `axis0.config.sensorless_ramp.vel` | float32 | read/write | 400.00000 | Unit: rad/s. |
| `axis0.config.startup_closed_loop_control` | bool | read/write | 0 | Enable closed loop control after calibration/startup |
| `axis0.config.startup_encoder_index_search` | bool | read/write | 0 | Run encoder index search after startup, skip otherwise this only has an effect if encoder.config.use_index is also true |
| `axis0.config.startup_encoder_offset_calibration` | bool | read/write | 0 | Run encoder offset calibration after startup, skip otherwise |
| `axis0.config.startup_homing` | bool | read/write | 0 | Enable homing after calibration/startup |
| `axis0.config.startup_motor_calibration` | bool | read/write | 0 | Run motor calibration at startup, skip otherwise |
| `axis0.config.step_dir_always_on` | bool | read/write | 0 | Keep step/dir enabled while the motor is disabled. This is ignored if enable_step_dir is false. This setting only takes effect on a state transition into idle or out of closed loop control. |
| `axis0.config.step_gpio_pin` | uint16 | read/write | 0 |  |
| `axis0.config.watchdog_timeout` | float32 | read/write | 0.000000 | 0 disables watchdog Unit: s. |

## Motor (`axis0.motor.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.motor.DC_calib_phA` | float32 | read/write | -2.654356 |  |
| `axis0.motor.DC_calib_phB` | float32 | read/write | -2.057165 |  |
| `axis0.motor.DC_calib_phC` | float32 | read/write | 4.711735 |  |
| `axis0.motor.I_bus` | float32 | read-only | 0.000000 | The current in the ODrive DC bus. This is also the current seen by the power supply in most systems. Unit: A. |
| `axis0.motor.current_control.I_measured_report_filter_k` | float32 | read/write | 1.000000 |  |
| `axis0.motor.current_control.Ialpha_measured` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.Ibeta_measured` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.Id_measured` | float32 | read-only | 0.000000 | The phase current measured along the "d" axis in the FOC control loop. This should be close to 0 for typical surface permanent magnet motors. Unit: A. |
| `axis0.motor.current_control.Id_setpoint` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.Iq_measured` | float32 | read-only | 0.000000 | The phase current measured along the "q" axis in the FOC control loop. This is the torque-generating current, so motor torque is approx. `torque_constant * Iq_measured` Unit: A. |
| `axis0.motor.current_control.Iq_setpoint` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.Vd_setpoint` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.Vq_setpoint` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.final_v_alpha` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.final_v_beta` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.i_gain` | float32 | read-only | 327.16842 |  |
| `axis0.motor.current_control.p_gain` | float32 | read-only | 0.104146 |  |
| `axis0.motor.current_control.phase` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.phase_vel` | float32 | read-only | 0.000000 |  |
| `axis0.motor.current_control.power` | float32 | read-only | 0.000000 | The electrical power being delivered to the motor Unit: W. |
| `axis0.motor.current_control.v_current_control_integral_d` | float32 | read/write | 0.000000 |  |
| `axis0.motor.current_control.v_current_control_integral_q` | float32 | read/write | 0.000000 |  |
| `axis0.motor.current_meas_phA` | float32 | read-only | 2.656309 |  |
| `axis0.motor.current_meas_phB` | float32 | read-only | 2.047986 |  |
| `axis0.motor.current_meas_phC` | float32 | read-only | -4.700834 |  |
| `axis0.motor.effective_current_lim` | float32 | read-only | 50.000000 | This value is the internally-limited value of phase current allowed according to the set current limit and the FET and Motor thermistor limits. Unit: A. |
| `axis0.motor.error` | flags | read/write | 0 (none) |  |
| `axis0.motor.is_armed` | bool | read-only | 0 |  |
| `axis0.motor.is_calibrated` | bool | read-only | 1 |  |
| `axis0.motor.last_error_time` | float32 | read/write | 0.000000 |  |
| `axis0.motor.max_allowed_current` | float32 | read-only | 148.49998 | Indicates the maximum current that can be measured by the current sensors in the current hardware configuration. This value depends on `config.requested_current_range`. Unit: A. |
| `axis0.motor.max_dc_calib` | float32 | read-only | 14.849998 | Unit: A. |
| `axis0.motor.n_evt_current_measurement` | uint32 | read-only | 4872020 | Number of current measurement events since startup (modulo 2^32) |
| `axis0.motor.n_evt_pwm_update` | uint32 | read-only | 4872103 | Number of PWM update events since startup (modulo 2^32) |
| `axis0.motor.phase_current_rev_gain` | float32 | read/write | 0.100000 |  |

## Motor configuration (`axis0.motor.config.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.motor.config.I_bus_hard_max` | float32 | read/write | inf | If the controller fails to keep this motor's DC current (`ODrive.Motor:I_bus`) below this value the motor gets disarmed immediately. Usually this is set in conjunction with `I_bus_hard_min`. Set to inf to disable. Unit: A. |
| `axis0.motor.config.I_bus_hard_min` | float32 | read/write | -inf | If the controller fails to keep this motor's DC current (`ODrive.Motor:I_bus`) above this value the motor gets disarmed immediately. Most likely you want a negative value here. Set to -inf to disable. Unit: A. |
| `axis0.motor.config.I_leak_max` | float32 | read/write | 0.100000 | In almost all scenarios, the currents on phase A, B and C should add up to zero. A small amount of measurement noise is expected. Unit: A. |
| `axis0.motor.config.R_wL_FF_enable` | bool | read/write | 0 | Enables automatic feedforward of the R*wL term in the current controller. |
| `axis0.motor.config.acim_autoflux_attack_gain` | float32 | read/write | 10.000000 |  |
| `axis0.motor.config.acim_autoflux_decay_gain` | float32 | read/write | 1.000000 |  |
| `axis0.motor.config.acim_autoflux_enable` | bool | read/write | 0 |  |
| `axis0.motor.config.acim_autoflux_min_Id` | float32 | read/write | 10.000000 |  |
| `axis0.motor.config.acim_gain_min_flux` | float32 | read/write | 10.000000 |  |
| `axis0.motor.config.bEMF_FF_enable` | bool | read/write | 0 | Enables automatic feedforward of the bEMF term in the current controller. |
| `axis0.motor.config.calibration_current` | float32 | read/write | 2.000000 | The current used to measure resistance during `AXIS_STATE_MOTOR_CALIBRATION`. Vendor manual: generally 2–5 A. |
| `axis0.motor.config.current_control_bandwidth` | float32 | read/write | 1000.0000 | Sets the PI gains of the Q and D axis FOC control according to `phase_resistance` and `phase_inductance` to create a critically-damped controller with a -3dB bandwidth at this frequency. Unit: rad/s. |
| `axis0.motor.config.current_lim` | float32 | read/write | 50.000000 | Maximum commanded current allowed Unit: A. Vendor manual: must not exceed 100. |
| `axis0.motor.config.current_lim_margin` | float32 | read/write | 8.000000 | Maximum measured current allowed above `current_lim`. Any value above `current_lim + curren_lim_margin` will throw a `CURRENT_LIMIT_VIOLATION` error. Unit: A. |
| `axis0.motor.config.dc_calib_tau` | float32 | read/write | 0.200000 |  |
| `axis0.motor.config.inverter_temp_limit_lower` | float32 | read/write | 100.00000 | Not implemented. Unit: °C. |
| `axis0.motor.config.inverter_temp_limit_upper` | float32 | read/write | 120.00000 | Not implemented. Unit: °C. |
| `axis0.motor.config.motor_type` | MotorType | read/write | 0 (HIGH_CURRENT) |  |
| `axis0.motor.config.phase_inductance` | float32 | read/write | 0.000104 | Unit: henry. |
| `axis0.motor.config.phase_resistance` | float32 | read/write | 0.327168 | Unit: ohm. |
| `axis0.motor.config.pole_pairs` | int32 | read/write | 14 | The number of pole pairs in the motor. Note this is equal to 1/2 of the number of magnets (not coils!) in a typical hobby motor. |
| `axis0.motor.config.pre_calibrated` | bool | read/write | 1 | Indicates to the ODrive that the parameters `config.phase_resistance` and `config.phase_inductance` are valid at system start. If these are valid and `pre_calibrated` is set to `True`, motor calibration can be skipped. |
| `axis0.motor.config.requested_current_range` | float32 | read/write | 60.000000 | The minimum phase current range expected to be measured. This is used to set the current shunt amplifier gains. Unit: A. |
| `axis0.motor.config.resistance_calib_max_voltage` | float32 | read/write | 7.000000 | The maximum voltage allowed during `AXIS_STATE_MOTOR_CALIBRATION`. Vendor manual: slightly less than half the supply voltage, e.g. 10 for 24 V. |
| `axis0.motor.config.torque_constant` | float32 | read/write | 0.430000 | Unit: N·m/A. |
| `axis0.motor.config.torque_lim` | float32 | read/write | inf | Maximum commanded torque allowed in the torque loop. Unit: N·m. |

## Thermistors (`axis0.motor.*_thermistor.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.motor.fet_thermistor.config.enabled` | bool | read/write | 0 | Whether this thermistor is enabled. |
| `axis0.motor.fet_thermistor.config.temp_limit_lower` | float32 | read/write | 25.000000 | The lower limit when the controller starts limiting current. |
| `axis0.motor.fet_thermistor.config.temp_limit_upper` | float32 | read/write | 80.000000 | The upper limit when current limit reaches 0 Amps and an over temperature error is triggered. |
| `axis0.motor.fet_thermistor.temperature` | float32 | read-only | 25.565182 | Unit: °C. |
| `axis0.motor.motor_thermistor.config.enabled` | bool | read/write | 0 | Whether this thermistor is enabled. |
| `axis0.motor.motor_thermistor.config.gpio_pin` | uint16 | read/write | 10 |  |
| `axis0.motor.motor_thermistor.config.poly_coefficient_0` | float32 | read/write | -296.7000 |  |
| `axis0.motor.motor_thermistor.config.poly_coefficient_1` | float32 | read/write | 504.29998 |  |
| `axis0.motor.motor_thermistor.config.poly_coefficient_2` | float32 | read/write | -375.7999 |  |
| `axis0.motor.motor_thermistor.config.poly_coefficient_3` | float32 | read/write | 141.30000 |  |
| `axis0.motor.motor_thermistor.config.temp_limit_lower` | float32 | read/write | 25.000000 | The lower limit when the controller starts limiting current. |
| `axis0.motor.motor_thermistor.config.temp_limit_upper` | float32 | read/write | 90.000000 | The upper limit when current limit reaches 0 Amps and an over temperature error is triggered. |
| `axis0.motor.motor_thermistor.temperature` | float32 | read-only | 26.031710 | Unit: °C. |
| `axis0.task_times.thermistor_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.thermistor_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.thermistor_update.max_length` | uint32 | read/write | 40 |  |
| `axis0.task_times.thermistor_update.start_time` | uint32 | read-only | 0 |  |

## Controller (`axis0.controller.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.controller.anticogging_valid` | bool | read/write | 0 |  |
| `axis0.controller.autotuning.frequency` | float32 | read/write | 0.000000 | Unit: Hz. |
| `axis0.controller.autotuning.pos_amplitude` | float32 | read/write | 0.000000 | Unit: turns. |
| `axis0.controller.autotuning.torque_amplitude` | float32 | read/write | 0.000000 | Unit: N·m. |
| `axis0.controller.autotuning.vel_amplitude` | float32 | read/write | 0.000000 | Unit: turns/sec. |
| `axis0.controller.autotuning_phase` | float32 | read/write | 0.000000 | The current phase angle of the `INPUT_MODE_TUNING` sine wave generator Unit: rad. |
| `axis0.controller.electrical_power` | float32 | read-only | 0.000000 | Electrical power estimate. Vdq·Idq Unit: Watt. |
| `axis0.controller.error` | flags | read/write | 0 (none) |  |
| `axis0.controller.input_mit_kd` | float32 | read/write | 0.000000 | MIT motion-control damping gain, Nm/(turn/s). Used with input mode 9. |
| `axis0.controller.input_mit_kp` | float32 | read/write | 0.000000 | MIT motion-control position gain, Nm/turn. Used with input mode 9. |
| `axis0.controller.input_pos` | float32 | read/write | 0.000000 | Set the desired position of the axis. Only valid in `CONTROL_MODE_POSITION_CONTROL`.startup. Also updates the step count. In `INPUT_MODE_TUNING`, this acts as a DC offset for the position sine wave. Unit: turn. |
| `axis0.controller.input_torque` | float32 | read/write | 0.000000 | In `CONTROL_MODE_TORQUE_CONTROL`, sets the desired output torque of the axis. In `CONTROL_MODE_VELOCITY_CONTROL` and `CONTROL_MODE_POSITION_CONTROL`, sets the feed-forward torque of the torque controller. Unit: N·m. |
| `axis0.controller.input_vel` | float32 | read/write | 0.000000 | In `CONTROL_MODE_VELOCITY_CONTROL`, sets the desired velocity of the axis. Unit: turn/s. |
| `axis0.controller.last_error_time` | float32 | read/write | 0.000000 |  |
| `axis0.controller.mechanical_power` | float32 | read-only | 0.000000 | Mechanical power estimate. Torque * velocity Unit: Watt. |
| `axis0.controller.pos_setpoint` | float32 | read-only | 0.000000 | The position reference actually being used by the position controller. This is the same as `input_pos` in `INPUT_MODE_PASSTHROUGH`, but may vary according to `InputMode`. Unit: turn. |
| `axis0.controller.torque_setpoint` | float32 | read-only | 0.000000 | The torque reference actually being used by the torque controller. This is the same as `input_vel` in `INPUT_MODE_PASSTHROUGH`, but may vary according to `InputMode`. Unit: N·m. |
| `axis0.controller.trajectory_done` | bool | read-only | 1 | Indicates the last commanded Trapezoidal Trajectory movement is complete. |
| `axis0.controller.vel_integrator_torque` | float32 | read/write | 0.000000 | The accumulated value of the velocity loop integrator Unit: N·m. |
| `axis0.controller.vel_setpoint` | float32 | read-only | 0.000000 | The velocity reference actually being used by the velocity controller. This is the same as `input_vel` in `INPUT_MODE_PASSTHROUGH`, but may vary according to `InputMode`. Unit: turn/s. |

## Controller configuration (`axis0.controller.config.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.controller.config.anticogging.anticogging_enabled` | bool | read/write | 1 |  |
| `axis0.controller.config.anticogging.calib_anticogging` | bool | read-only | 0 |  |
| `axis0.controller.config.anticogging.calib_pos_threshold` | float32 | read/write | 1.000000 |  |
| `axis0.controller.config.anticogging.calib_vel_threshold` | float32 | read/write | 1.000000 |  |
| `axis0.controller.config.anticogging.cogging_ratio` | float32 | read-only | 1.000000 |  |
| `axis0.controller.config.anticogging.index` | uint32 | read-only | 0 |  |
| `axis0.controller.config.anticogging.pre_calibrated` | bool | read/write | 0 |  |
| `axis0.controller.config.axis_to_mirror` | uint8 | read/write | 255 | The axis used for mirroring when in `INPUT_MODE_MIRROR` |
| `axis0.controller.config.circular_setpoint_range` | float32 | read/write | 1.000000 | circular range in [turns] for position setpoints when circular_setpoints is True |
| `axis0.controller.config.circular_setpoints` | bool | read/write | 0 |  |
| `axis0.controller.config.control_mode` | ControlMode | read/write | 2 (VELOCITY_CONTROL) |  |
| `axis0.controller.config.electrical_power_bandwidth` | float32 | read/write | 20.000000 | Bandwidth for electrical power estimate. Used for spinout detection. Dot product of Vdq and Idq Unit: rad/s. |
| `axis0.controller.config.enable_gain_scheduling` | bool | read/write | 0 | Enable the experimental "gain scheduling" module, which reduces the `pos_gain`, `vel_gain`, and `vel_integrator_gain` according to the position error. Also known as "anti-hunt" |
| `axis0.controller.config.enable_overspeed_error` | bool | read/write | 0 | Enables the velocity controller's overspeed error |
| `axis0.controller.config.enable_torque_mode_vel_limit` | bool | read/write | 0 | Enable velocity limit in torque control mode (requires a valid velocity estimator). |
| `axis0.controller.config.enable_vel_limit` | bool | read/write | 0 |  |
| `axis0.controller.config.gain_scheduling_width` | float32 | read/write | 10.000000 |  |
| `axis0.controller.config.homing_speed` | float32 | read/write | 0.250000 | The speed at which the axis moves towards the `min_endstop` during `AXIS_STATE_HOMING` Unit: turn/s. |
| `axis0.controller.config.inertia` | float32 | read/write | 0.500000 | Unit: N·m/(turn/s^2). |
| `axis0.controller.config.input_filter_bandwidth` | float32 | read/write | 2.000000 | The desired bandwidth for `INPUT_MODE_POS_FILTER`. Unit: rad/s. |
| `axis0.controller.config.input_mode` | InputMode | read/write | 1 (PASSTHROUGH) | Vendor manual adds 9 = motion control (MIT). |
| `axis0.controller.config.load_encoder_axis` | uint8 | read/write | 0 | Default depends on Axis number and is set in load_configuration() |
| `axis0.controller.config.mechanical_power_bandwidth` | float32 | read/write | 20.000000 | Bandwidth for mechanical power estimate. Used for spinout detection Unit: rad/s. |
| `axis0.controller.config.mirror_ratio` | float32 | read/write | 1.000000 | The ratio applied to position and velocity movements of the mirrored axis. To reverse movements, use a negative value |
| `axis0.controller.config.pos_gain` | float32 | read/write | 20.000000 | units = (turn/s) / turn Unit: (turn/s) / turn. |
| `axis0.controller.config.spinout_electrical_power_threshold` | float32 | read/write | 10.000000 | Electrical power threshold for spinout detection. This should be a positive value Unit: Watt. |
| `axis0.controller.config.spinout_mechanical_power_threshold` | float32 | read/write | -10.00000 | Mechanical power threshold for spinout detection. This should be a negative value Unit: Watt. |
| `axis0.controller.config.steps_per_circular_range` | int32 | read/write | 1024 | Number of steps within the circular setpoint range. Set this and the circular setpoint range to powers of 2 for the best results. |
| `axis0.controller.config.torque_mirror_ratio` | float32 | read/write | 0.000000 | The ratio applied to torque values of the mirrored axis. |
| `axis0.controller.config.torque_ramp_rate` | float32 | read/write | 0.010000 | Unit: N·m / sec. |
| `axis0.controller.config.vel_gain` | float32 | read/write | 0.166667 | units = N·m / (turn/s) Unit: N·m / (turn/s). |
| `axis0.controller.config.vel_integrator_gain` | float32 | read/write | 0.333333 | units = N·m / (turn/s * s) Unit: N·m / (turn/s * s). |
| `axis0.controller.config.vel_integrator_limit` | float32 | read/write | inf | Limit the integrator output (independent of proportional gain output). Set to infinity to disable. Units = N·m Unit: N·m. |
| `axis0.controller.config.vel_limit` | float32 | read/write | 30.000000 | Infinity to disable. Unit: turn/s. |
| `axis0.controller.config.vel_limit_tolerance` | float32 | read/write | 1.200000 | Ratio to `vel_limit`. Infinity to disable. |
| `axis0.controller.config.vel_ramp_rate` | float32 | read/write | 1.000000 | Unit: turn/s^2. |

## Encoder (`axis0.encoder.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.encoder.calib_scan_response` | float32 | read-only | 0.000000 |  |
| `axis0.encoder.count_in_cpr` | int32 | read-only | 2768 | Raw circular count from the encoder on [0, cpr) Unit: counts. |
| `axis0.encoder.delta_pos_cpr_counts` | float32 | read-only | 0.227494 | Circular position delta of the encoder in the most recent loop. Primarily for debug purposes, it indicates much the encoder changed since the last time it was checked. Unit: counts. |
| `axis0.encoder.error` | flags | read/write | 0 (none) |  |
| `axis0.encoder.hall_state` | uint8 | read-only | 0 |  |
| `axis0.encoder.index_found` | bool | read-only | 0 |  |
| `axis0.encoder.interpolation` | float32 | read-only | 0.765625 |  |
| `axis0.encoder.is_ready` | bool | read-only | 1 |  |
| `axis0.encoder.phase` | float32 | read-only | -2.457880 |  |
| `axis0.encoder.pos_abs` | int32 | read/write | 2769 | The last (valid) position from an absolute encoder, if used. |
| `axis0.encoder.pos_circular` | float32 | read-only | 0.168938 | Circular position estimate of the encoder, as a decimal from [0, 1). Also known as "single-turn" position. Unit: turns. |
| `axis0.encoder.pos_cpr_counts` | float32 | read-only | 2769.1093 | Circular position estimate of the encoder, on the space [0, cpr). Unit: counts. |
| `axis0.encoder.pos_estimate` | float32 | read-only | -3.831015 | Linear position estimate of the encoder, in turns. Also known as "multi-turn" position. Unit: turns. |
| `axis0.encoder.pos_estimate_counts` | float32 | read-only | -62766.71 | Linear position estimate of the encoder, in counts. Equal to `pos_estimate * config.cpr` Unit: counts. |
| `axis0.encoder.shadow_count` | int32 | read-only | -62769 | Raw linear count from the encoder. Unit: counts. |
| `axis0.encoder.spi_error_rate` | float32 | read-only | 0.000000 |  |
| `axis0.encoder.vel_estimate` | float32 | read-only | -0.007629 | Estimate of the linear velocity of an axis in turn/s Unit: turn/s. |
| `axis0.encoder.vel_estimate_counts` | float32 | read-only | 1000.0000 | Estimate of the linear velocity of an axis, in counts/s. Unit: counts/sec. |

## Encoder configuration (`axis0.encoder.config.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.encoder.config.abs_spi_cs_gpio_pin` | uint16 | read/write | 11 | Make sure that the GPIO is in `GPIO_MODE_DIGITAL`. |
| `axis0.encoder.config.bandwidth` | float32 | read/write | 1000.0000 | Unit: rad/s. |
| `axis0.encoder.config.calib_range` | float32 | read/write | 0.020000 | The error threshold for encoder offset calibration, in turns. If the Unit: turn. |
| `axis0.encoder.config.calib_scan_distance` | float32 | read/write | 50.265484 | The distance the motor is turned during encoder offset calibration. This is in the electrical frame, so if you want a 7pp motor to turn 1 mechanical revolution, you would put `(7 * 2*pi)` Unit: rad (electrical). |
| `axis0.encoder.config.calib_scan_omega` | float32 | read/write | 12.566371 | The speed the motor turns during encoder offset calibration. This is in the electrical frame, so a 7pp motor rotating 1 mechanical revolution in 2 sec would be set to `(7 * 2*pi) / 2` Unit: rad/s (electrical). |
| `axis0.encoder.config.cpr` | int32 | read/write | 16384 | Counts per Revolution of the encoder. This is 4x the Pulses per Revolution. |
| `axis0.encoder.config.direction` | int32 | read/write | -1 |  |
| `axis0.encoder.config.enable_phase_interpolation` | bool | read/write | 1 |  |
| `axis0.encoder.config.find_idx_on_lockin_only` | bool | read/write | 0 |  |
| `axis0.encoder.config.hall_polarity` | uint8 | read/write | 0 |  |
| `axis0.encoder.config.hall_polarity_calibrated` | bool | read/write | 0 |  |
| `axis0.encoder.config.ignore_illegal_hall_state` | bool | read/write | 0 | Ignore the error "Illegal Hall State" |
| `axis0.encoder.config.index_offset` | float32 | read/write | 0.000000 | When the index is found, the linear position of the axis will be set to this value if `use_index_offset` is set to `True`. Vendor manual: user zero offset, relative to the encoder zero. Position targets are referenced to it once saved. |
| `axis0.encoder.config.mode` | Mode | read/write | 260 (SPI_ABS_MA732) |  |
| `axis0.encoder.config.phase_offset` | int32 | read/write | 2310 |  |
| `axis0.encoder.config.phase_offset_float` | float32 | read/write | 1.203213 |  |
| `axis0.encoder.config.pre_calibrated` | bool | read/write | 1 |  |
| `axis0.encoder.config.sincos_gpio_pin_cos` | uint16 | read/write | 4 | Analog cosine signal of a sin/cos encoder. The corresponding GPIO must be in `GPIO_MODE_ANALOG_IN`. |
| `axis0.encoder.config.sincos_gpio_pin_sin` | uint16 | read/write | 3 | Analog sine signal of a sin/cos encoder. The corresponding GPIO must be in `GPIO_MODE_ANALOG_IN`. |
| `axis0.encoder.config.use_index` | bool | read/write | 0 | Set this to `True` when using an encoder with an index pin to allow pre-calibration of the encoder and encoder index search. |
| `axis0.encoder.config.use_index_offset` | bool | read/write | 1 | Set this to `True` to have the axis set the linear position of the axis to `index_offset` when the index is found during encoder index search |

## Trajectory planner (`axis0.trap_traj.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.trap_traj.config.accel_limit` | float32 | read/write | 0.500000 | Unit: turn/s^2. |
| `axis0.trap_traj.config.decel_limit` | float32 | read/write | 0.500000 | Unit: turn/s^2. |
| `axis0.trap_traj.config.vel_limit` | float32 | read/write | 2.000000 | Unit: turn/s. |

## Endstops and brake (`axis0.min_endstop.`, `axis0.max_endstop.`, `axis0.mechanical_brake.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.max_endstop.config.debounce_ms` | uint32 | read/write | 50 |  |
| `axis0.max_endstop.config.enabled` | bool | read/write | 0 |  |
| `axis0.max_endstop.config.gpio_num` | uint16 | read/write | 0 | Make sure the corresponding GPIO is in `GPIO_MODE_DIGITAL`. Vendor manual: set to 2 for the maximum limit switch (LW2). |
| `axis0.max_endstop.config.is_active_high` | bool | read/write | 0 |  |
| `axis0.max_endstop.config.offset` | float32 | read/write | 0.000000 | Unit: turns. |
| `axis0.max_endstop.endstop_state` | bool | read-only | 0 |  |
| `axis0.mechanical_brake.config.gpio_num` | uint16 | read/write | 0 |  |
| `axis0.mechanical_brake.config.is_active_low` | bool | read/write | 1 |  |
| `axis0.min_endstop.config.debounce_ms` | uint32 | read/write | 50 |  |
| `axis0.min_endstop.config.enabled` | bool | read/write | 0 |  |
| `axis0.min_endstop.config.gpio_num` | uint16 | read/write | 0 | Make sure the corresponding GPIO is in `GPIO_MODE_DIGITAL`. Vendor manual: set to 1 for the minimum limit switch (LW1). |
| `axis0.min_endstop.config.is_active_high` | bool | read/write | 0 |  |
| `axis0.min_endstop.config.offset` | float32 | read/write | 0.000000 | Unit: turns. |
| `axis0.min_endstop.endstop_state` | bool | read-only | 0 |  |

## Sensorless estimator and ACIM (`axis0.sensorless_estimator.`, `axis0.acim_estimator.`)

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.acim_estimator.config.slip_velocity` | float32 | read/write | 14.706000 |  |
| `axis0.acim_estimator.phase_offset` | float32 | read-only | 0.000000 | estimate offset between physical and electrical angular position} Unit: rad. |
| `axis0.acim_estimator.rotor_flux` | float32 | read-only | 0.000000 | estimated magnitude of the rotor flux Unit: A. |
| `axis0.acim_estimator.slip_vel` | float32 | read-only | 0.000000 | estimated slip between physical and electrical angular velocity} Unit: rad/s. |
| `axis0.acim_estimator.stator_phase` | float32 | read-only | 0.000000 | calculated setpoint for the electrical phase} Unit: rad. |
| `axis0.acim_estimator.stator_phase_vel` | float32 | read-only | 0.000000 | calculated setpoint for the electrical velocity} Unit: rad/s. |
| `axis0.sensorless_estimator.config.observer_gain` | float32 | read/write | 1000.0000 |  |
| `axis0.sensorless_estimator.config.pll_bandwidth` | float32 | read/write | 1000.0000 |  |
| `axis0.sensorless_estimator.config.pm_flux_linkage` | float32 | read/write | 0.001580 |  |
| `axis0.sensorless_estimator.error` | flags | read/write | 0 (none) |  |
| `axis0.sensorless_estimator.phase` | float32 | read-only | 0.000000 | Unit: rad. |
| `axis0.sensorless_estimator.phase_vel` | float32 | read-only | 0.000000 | Unit: rad/s. |
| `axis0.sensorless_estimator.pll_pos` | float32 | read-only | 0.000000 | Unit: rad. |
| `axis0.sensorless_estimator.vel_estimate` | float32 | read-only | 0.000000 | Unit: turn/s. |

## Task timing and oscilloscope

| Property | Type | Access | Value | Description |
|---|---|---|---|---|
| `axis0.task_times.acim_estimator_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.acim_estimator_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.acim_estimator_update.max_length` | uint32 | read/write | 0 |  |
| `axis0.task_times.acim_estimator_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.can_heartbeat.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.can_heartbeat.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.can_heartbeat.max_length` | uint32 | read/write | 0 |  |
| `axis0.task_times.can_heartbeat.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.controller_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.controller_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.controller_update.max_length` | uint32 | read/write | 1044 |  |
| `axis0.task_times.controller_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.current_controller_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.current_controller_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.current_controller_update.max_length` | uint32 | read/write | 392 |  |
| `axis0.task_times.current_controller_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.current_sense.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.current_sense.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.current_sense.max_length` | uint32 | read/write | 296 |  |
| `axis0.task_times.current_sense.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.dc_calib.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.dc_calib.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.dc_calib.max_length` | uint32 | read/write | 124 |  |
| `axis0.task_times.dc_calib.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.encoder_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.encoder_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.encoder_update.max_length` | uint32 | read/write | 1228 |  |
| `axis0.task_times.encoder_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.endstop_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.endstop_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.endstop_update.max_length` | uint32 | read/write | 120 |  |
| `axis0.task_times.endstop_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.motor_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.motor_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.motor_update.max_length` | uint32 | read/write | 444 |  |
| `axis0.task_times.motor_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.open_loop_controller_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.open_loop_controller_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.open_loop_controller_update.max_length` | uint32 | read/write | 364 |  |
| `axis0.task_times.open_loop_controller_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.pwm_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.pwm_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.pwm_update.max_length` | uint32 | read/write | 368 |  |
| `axis0.task_times.pwm_update.start_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.sensorless_estimator_update.end_time` | uint32 | read-only | 0 |  |
| `axis0.task_times.sensorless_estimator_update.length` | uint32 | read-only | 0 |  |
| `axis0.task_times.sensorless_estimator_update.max_length` | uint32 | read/write | 0 |  |
| `axis0.task_times.sensorless_estimator_update.start_time` | uint32 | read-only | 0 |  |
| `oscilloscope.size` | uint32 | read-only | 4096 |  |
| `task_times.control_loop_checks.end_time` | uint32 | read-only | 0 |  |
| `task_times.control_loop_checks.length` | uint32 | read-only | 0 |  |
| `task_times.control_loop_checks.max_length` | uint32 | read/write | 424 |  |
| `task_times.control_loop_checks.start_time` | uint32 | read-only | 0 |  |
| `task_times.control_loop_misc.end_time` | uint32 | read-only | 0 |  |
| `task_times.control_loop_misc.length` | uint32 | read-only | 0 |  |
| `task_times.control_loop_misc.max_length` | uint32 | read/write | 756 |  |
| `task_times.control_loop_misc.start_time` | uint32 | read-only | 0 |  |
| `task_times.dc_calib_wait.end_time` | uint32 | read-only | 0 |  |
| `task_times.dc_calib_wait.length` | uint32 | read-only | 0 |  |
| `task_times.dc_calib_wait.max_length` | uint32 | read/write | 4048 |  |
| `task_times.dc_calib_wait.start_time` | uint32 | read-only | 0 |  |
| `task_times.sampling.end_time` | uint32 | read-only | 0 |  |
| `task_times.sampling.length` | uint32 | read-only | 0 |  |
| `task_times.sampling.max_length` | uint32 | read/write | 740 |  |
| `task_times.sampling.start_time` | uint32 | read-only | 0 |  |

## Vendor additions

These are in the Steadywin firmware but not in stock ODrive 0.5.6.

| Property | Value | Description |
|---|---|---|
| `axis0.controller.input_mit_kd` | 0.000000 | MIT motion-control damping gain, Nm/(turn/s). Used with input mode 9. |
| `axis0.controller.input_mit_kp` | 0.000000 | MIT motion-control position gain, Nm/turn. Used with input mode 9. |
| `can.config.enable_r120` | 1 | Switches the on-board 120 Ω CAN termination resistor. |
| `can.config.r120_gpio_num` | 5 | GPIO number that drives the termination resistor switch. The manual uses 5. |

## Enumerations

Names and numbers are from stock 0.5.6. The vendor firmware may assign other meanings; input mode 9 (MIT) is one known addition.

### Axis.AxisState

| Value | Name | Meaning |
|---|---|---|
| 0 | UNDEFINED | will fall through to idle |
| 1 | IDLE | Disable motor PWM and do nothing. |
| 2 | STARTUP_SEQUENCE | Run the startup procedure. |
| 3 | FULL_CALIBRATION_SEQUENCE | Run motor calibration and then encoder offset calibration (or encoder index search if `<axis>.encoder.config.use_index` is `True`). |
| 4 | MOTOR_CALIBRATION | Measure phase resistance and phase inductance of the motor. |
| 6 | ENCODER_INDEX_SEARCH | Turn the motor in one direction until the encoder index is traversed. |
| 7 | ENCODER_OFFSET_CALIBRATION | Turn the motor in one direction for a few seconds and then back to measure the offset between the encoder position and the electrical phase. |
| 8 | CLOSED_LOOP_CONTROL | Run closed loop control. |
| 9 | LOCKIN_SPIN | Run lockin spin. |
| 10 | ENCODER_DIR_FIND | Run encoder direction search. |
| 11 | HOMING | Run axis homing function. |
| 12 | ENCODER_HALL_POLARITY_CALIBRATION | Rotate the motor in lockin and calibrate hall polarity |
| 13 | ENCODER_HALL_PHASE_CALIBRATION | Rotate the motor for 30s to calibrate hall sensor edge offsets |

### Can.Protocol

| Value | Name | Meaning |
|---|---|---|
| 0x1 | SIMPLE | CANSimple, an ODrive-specific protocol for basic functionality |

### Controller.ControlMode

| Value | Name | Meaning |
|---|---|---|
| 0 | VOLTAGE_CONTROL | Note: This mode is not used internally. For voltage-only FOC, use `MotorType.GIMBAL` |
| 1 | TORQUE_CONTROL | Uses only the inner torque control loop. Use `input_torque` to command desired torque. Note the setting `motor.config.torque_constant`. |
| 2 | VELOCITY_CONTROL | Uses both the inner torque control loop and the velocity control loop. Use `input_vel` to command desired velocity, and `input_torque`. |
| 3 | POSITION_CONTROL | Uses the inner torque loop, the velocity control loop, and the outer position control loop. |

### Controller.InputMode

| Value | Name | Meaning |
|---|---|---|
| 0 | INACTIVE | Disable inputs. Setpoints retain their last value. |
| 1 | PASSTHROUGH | Pass `input_xxx` through to `xxx_setpoint` directly. |
| 2 | VEL_RAMP | Ramps a velocity command from the current value to the target value. |
| 3 | POS_FILTER | Implements a 2nd order position tracking filter. |
| 4 | MIX_CHANNELS | Not Implemented. |
| 5 | TRAP_TRAJ | Implementes an online trapezoidal trajectory planner. |
| 6 | TORQUE_RAMP | Ramp a torque command from the current value to the target value. |
| 7 | MIRROR | Implements "electronic mirroring". |
| 8 | TUNING | Implements a tuning mode |

### Encoder.Mode

| Value | Name | Meaning |
|---|---|---|
| 0 | INCREMENTAL |  |
| 1 | HALL |  |
| 2 | SINCOS |  |
| 256 | SPI_ABS_CUI | Compatible with CUI AMT23xx |
| 257 | SPI_ABS_AMS | Compatible with AMS AS5047P, AS5048A/AS5048B (no daisy chain support) |
| 258 | SPI_ABS_AEAT | Supports AEAT-8800 |
| 259 | SPI_ABS_RLS | Supports RLS Orbis Encoders |
| 260 | SPI_ABS_MA732 | MagAlpha MA732 magnetic encoder |

### GpioMode

| Value | Name | Meaning |
|---|---|---|
| 0 | DIGITAL | The pin can be used for one or more of these functions: Step, dir, enable, encoder index, hall effect encoder, SPI encoder nCS (this one is exclusive). |
| 1 | DIGITAL_PULL_UP | Same as `DIGITAL` but with the internal pull-up resistor enabled. |
| 2 | DIGITAL_PULL_DOWN | Same as `DIGITAL` but with the internal pull-down resistor enabled. |
| 3 | ANALOG_IN | The pin can be used for one or more of these functions: Sin/cos encoders, analog input, `get_adc_voltage()`. |
| 4 | UART_A | See `config.enable_uart_a`. |
| 5 | UART_B | This mode is not supported on ODrive v3.x. |
| 6 | UART_C | This mode is not supported on ODrive v3.x. |
| 7 | CAN_A | See `config.enable_can_a`. |
| 8 | I2C_A | See `config.enable_i2c_a`. |
| 9 | SPI_A | Note that the SPI pins on ODrive v3.x are hardwired so they cannot be configured through software. Consequently |
| 10 | PWM | See `config.gpio1_pwm_mapping`. |
| 11 | ENC0 | The pin is used by quadrature encoder 0. |
| 12 | ENC1 | The pin is used by quadrature encoder 1. |
| 13 | ENC2 | This mode is not supported on ODrive v3.x. |
| 14 | MECH_BRAKE | This is to support external mechanical brakes. |
| 15 | STATUS | The pin is used for status output (see `config.error_gpio_pin`) |

### Motor.MotorType

| Value | Name | Meaning |
|---|---|---|
| 0 | HIGH_CURRENT | Used for all standard "hobby-style" motors Permanant Magnet AC (PMAC) or Brushless DC (BLDC) motors. This is the default. |
| 2 | GIMBAL | Enables voltage-only FOC where V=IR can be assumed to be correct. Used for high-phase-resistance motors (> 1 ohm), which are typically sold as "Gimbal" motors. |
| 3 | ACIM | Used for FOC control of AC Induction Motors (ACIM), aka Asynchronous motors, |

### StreamProtocolType

| Value | Name | Meaning |
|---|---|---|
| 0 | Fibre | Machine-to-machine protocol which gives access to all features of the ODrive. This protocol is used by the official odrivetool and GUI. |
| 1 | Ascii | Human readable protocol designed for easy implementation for cases where the use of libfibre is not desired or feasible. |
| 2 | Stdout | Output of printf(). Only intended for developers who modify ODrive firmware. |
| 3 | AsciiAndStdout | Combination of `Ascii` and `Stdout`. |

## Error flags

Error properties are bit fields; a value is the sum of the flags that are set.

### `error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | CONTROL_ITERATION_MISSED | At least one control iteration was missed. |
| 0x2 | DC_BUS_UNDER_VOLTAGE | The DC voltage fell below the limit configured in `config.dc_bus_undervoltage_trip_level`. |
| 0x4 | DC_BUS_OVER_VOLTAGE | The DC voltage exceeded the limit configured in `config.dc_bus_overvoltage_trip_level`. |
| 0x8 | DC_BUS_OVER_REGEN_CURRENT | Current flowing back into the power supply exceeded `config.dc_max_negative_current`. |
| 0x10 | DC_BUS_OVER_CURRENT | Too much current was pulled from the power supply. `ibus` exceeded `config.dc_max_positive_current`. |
| 0x20 | BRAKE_DEADTIME_VIOLATION |  |
| 0x40 | BRAKE_DUTY_CYCLE_NAN |  |
| 0x80 | INVALID_BRAKE_RESISTANCE | `config.brake_resistance` is non-positive or NaN. Make sure that `config.brake_resistance` is a positive number. |

### `can.error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | DUPLICATE_CAN_IDS |  |

### `axis0.error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | INVALID_STATE | An invalid state was requested. |
| 0x40 | MOTOR_FAILED | Check `motor.error` for more information. |
| 0x80 | SENSORLESS_ESTIMATOR_FAILED |  |
| 0x100 | ENCODER_FAILED | Check `encoder.error` for more information. |
| 0x200 | CONTROLLER_FAILED |  |
| 0x800 | WATCHDOG_TIMER_EXPIRED | The axis watchdog timer expired. |
| 0x1000 | MIN_ENDSTOP_PRESSED | The min endstop was pressed |
| 0x2000 | MAX_ENDSTOP_PRESSED | The max endstop was pressed |
| 0x4000 | ESTOP_REQUESTED | The estop message was sent over CAN |
| 0x20000 | HOMING_WITHOUT_ENDSTOP | the min endstop was not enabled during homing |
| 0x40000 | OVER_TEMP | Check `motor.error` for more details. |
| 0x80000 | UNKNOWN_POSITION | There isn't a valid position estimate available. |

### `axis0.motor.error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | PHASE_RESISTANCE_OUT_OF_RANGE | The measured motor phase resistance is outside of the plausible range. |
| 0x2 | PHASE_INDUCTANCE_OUT_OF_RANGE | The measured motor phase inductance is outside of the plausible range. |
| 0x8 | DRV_FAULT | The gate driver chip reported an error. |
| 0x10 | CONTROL_DEADLINE_MISSED |  |
| 0x80 | MODULATION_MAGNITUDE | The bus voltage was insufficent to push the requested current through the motor. |
| 0x400 | CURRENT_SENSE_SATURATION | The current sense circuit saturated the current sense amplifier. |
| 0x1000 | CURRENT_LIMIT_VIOLATION | The motor current exceeded `motor.config.current_lim + motor.config.current_lim_margin`. |
| 0x10000 | MODULATION_IS_NAN |  |
| 0x20000 | MOTOR_THERMISTOR_OVER_TEMP | The motor thermistor measured a temperature above motor.motor_thermistor.config.temp_limit_upper |
| 0x40000 | FET_THERMISTOR_OVER_TEMP | The inverter thermistor measured a temperature above motor.fet_thermistor.config.temp_limit_upper |
| 0x80000 | TIMER_UPDATE_MISSED | A timer update event was missed. Perhaps the previous timer update took too much time. This is not expected in official release firmware. |
| 0x100000 | CURRENT_MEASUREMENT_UNAVAILABLE | The phase current measurement is not available. The ADC failed to sample the current sensor in time. This is not expected in official release firmware. |
| 0x200000 | CONTROLLER_FAILED | The motor was disarmed because the underlying controller failed. Usually this is the FOC controller. |
| 0x400000 | I_BUS_OUT_OF_RANGE | The DC current sourced/sunk by this motor exceeded the configured hard limits. More specifically `I_bus` fell outside of the range `config.I_bus_hard_min` ... |
| 0x800000 | BRAKE_RESISTOR_DISARMED | An attempt was made to run the motor PWM while the brake resistor was configured as enabled (`config.enable_brake_resistor`) but disarmed. |
| 0x1000000 | SYSTEM_LEVEL | The motor had to be disarmed because of a system level error. See `ODrive:error` for more details. |
| 0x2000000 | BAD_TIMING | The main control loop got out of sync with the motor control loop. This could indicate that the main control loop got stuck. |
| 0x4000000 | UNKNOWN_PHASE_ESTIMATE | The current controller did not get a valid angle input. Maybe you didn't calibrate the encoder. |
| 0x8000000 | UNKNOWN_PHASE_VEL | The motor controller did not get a valid phase velocity input. |
| 0x10000000 | UNKNOWN_TORQUE | The motor controller did not get a valid torque input. |
| 0x20000000 | UNKNOWN_CURRENT_COMMAND | The current controller did not get a valid current setpoint. Maybe you didn't configure the controller correctly. |
| 0x40000000 | UNKNOWN_CURRENT_MEASUREMENT | The current controller did not get a valid current measurement. |
| 0x80000000 | UNKNOWN_VBUS_VOLTAGE | The current controller did not get a valid `ODrive:vbus_voltage` measurement. |
| 0x100000000 | UNKNOWN_VOLTAGE_COMMAND | The current controller did not get a valid feedforward voltage setpoint. |
| 0x200000000 | UNKNOWN_GAINS | The current controller gains were not configured. Run motor calibration or set `config.phase_resistance` and `config.phase_inductance` manually. |
| 0x400000000 | CONTROLLER_INITIALIZING | Internal value used while the controller is not yet ready to generate PWM timings. |
| 0x800000000 | UNBALANCED_PHASES | The motor phases are not balanced. |

### `axis0.encoder.error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | UNSTABLE_GAIN |  |
| 0x2 | CPR_POLEPAIRS_MISMATCH | Confirm you have entered the correct count per rotation (CPR) for [your encoder](https://docs.odriverobotics.com/encoders). |
| 0x4 | NO_RESPONSE | Confirm that your encoder is plugged into the right pins on the ODrive board. |
| 0x8 | UNSUPPORTED_ENCODER_MODE |  |
| 0x10 | ILLEGAL_HALL_STATE | Hall effect encoder only have 6 valid states out of 8 (2^3) possible states. An invalid state can be caused by noise or a hardware fault. |
| 0x20 | INDEX_NOT_FOUND_YET | Check that your encoder is a model that has an index pulse. If your encoder does not have a wire connected to pin Z on your ODrive then it does not output an… |
| 0x40 | ABS_SPI_TIMEOUT |  |
| 0x80 | ABS_SPI_COM_FAIL |  |
| 0x100 | ABS_SPI_NOT_READY |  |
| 0x200 | HALL_NOT_CALIBRATED_YET |  |

### `axis0.controller.error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | OVERSPEED | Motor speed exceeded `config.vel_limit * config.vel_limit_tolerance` and `config.enable_overspeed_error` was enabled. |
| 0x2 | INVALID_INPUT_MODE | The `config.input_mode` setting was set to an invalid value. See InputMode for available values |
| 0x4 | UNSTABLE_GAIN | `motor.config.bandwidth` was set too high to create a stable controller. |
| 0x8 | INVALID_MIRROR_AXIS | An invalid `axis_to_mirror` was selected |
| 0x10 | INVALID_LOAD_ENCODER | An invalid `load_encoder_axis` was selected |
| 0x20 | INVALID_ESTIMATE | Indicates the encoder estimation module declined to output a linear position or velocity value and the exception was caught. |
| 0x40 | INVALID_CIRCULAR_RANGE | Indicates the encoder estimation module declined to output a circular position value and the exception was caught. |
| 0x80 | SPINOUT_DETECTED | The motor mechanical power and electrical power do not agree. This is usually caused by a slipping encoder or incorrect encoder offset calibration. |

### `axis0.sensorless_estimator.error`

| Bit | Name | Meaning |
|---|---|---|
| 0x1 | UNSTABLE_GAIN |  |
| 0x2 | UNKNOWN_CURRENT_MEASUREMENT |  |

The vendor manual lists one extra encoder error, `SEC_ENC_COM_FAIL` (0x40, second encoder communication error).

## Functions

Functions from stock 0.5.6. They are called from `odrivetool`, not through ASCII property reads, so they were not checked on this driver.

| Function | Arguments | Description |
|---|---|---|
| `axis0.controller.get_anticogging_value()` | index |  |
| `axis0.controller.move_incremental()` | displacement, from_input_pos | Moves the axes' goal point by a specified increment. |
| `axis0.controller.remove_anticogging_bias()` | none |  |
| `axis0.controller.start_anticogging_calibration()` | none |  |
| `axis0.encoder.set_linear_count()` | count | Set the current linear position of the axis, in counts |
| `axis0.mechanical_brake.engage()` | none | This function engages the mechanical brake if one is present and enabled. |
| `axis0.mechanical_brake.release()` | none | This function releases the mecahncal brake if one is present and enabled. |
| `axis0.watchdog_feed()` | none | Feed the watchdog to prevent watchdog timeouts. |
| `clear_errors()` | none | Clear all the errors of this device including all contained submodules. |
| `enter_dfu_mode()` | none | Enters the Device Firmware Update mode |
| `erase_configuration()` | none | Resets all `config` variables to their default values and reboots the controller |
| `get_adc_voltage()` | gpio | Reads the ADC voltage of the specified GPIO. The GPIO should be in `GPIO_MODE_ANALOG_IN`.} |
| `get_dma_status()` | stream_num | Returns information about the specified DMA stream. |
| `get_drv_fault()` | none |  |
| `get_gpio_states()` | none | Returns the logic states of all GPIOs. Bit i represents the state of GPIOi. |
| `get_interrupt_status()` | irqn | Returns information about the specified interrupt number. |
| `oscilloscope.get_val()` | index |  |
| `reboot()` | none | Reboots the controller without saving the current configuraiton |
| `save_configuration()` | none | Saves the current configuration to non-volatile memory and reboots the board. |
| `test_function()` | delta |  |
