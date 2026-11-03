# roboparty_pmic project layout

The project follows the Roboparty GD32 USB-CAN layout while retaining the
RT-Thread 5.3.0 BSP build baseline:

- `common/bsp`: board abstraction (`bsp_pin_*`, UART device names, ADC/PWM/Flash)
- `common/app`: shared application sources and headers
- `common/drivers`: chip-independent drivers (CH224, TMP100, OLED, TWS BMS)
- `boards/gd32f503_pmic`: GD32F503 BSP, pinout, linker and RT-Thread port
- `boards/stm32f103_pmic`: STM32F103 BSP, pinout, linker and RT-Thread port
- `drivers`: RT-Thread peripheral driver layer (GD32 legacy path)
- `inc`: application and board headers (GD32 legacy path)
- `src`: application sources (GD32 legacy path)
- `scripts`: project helper scripts
- `thirdparty/GD32F50x_Firmware_Library_V1.0.3`: exact F503 vendor SDK
- `thirdparty/STM32CubeF1-v1.8.7.zip`: ST CMSIS core package (vendored)
- `thirdparty/cmsis_device_f1-v4.3.5.zip`: ST STM32F1 device headers (vendored)
- `thirdparty/STM32F1xx_CMSIS`: extracted locally by
  `scripts/extract_stm32_cmsis.sh` (gitignored)
- `thirdparty/rt-thread`: RT-Thread 5.3.0 source (local clone / `RTT_ROOT`,
  shared by `boards/gd32f503_pmic` and `boards/stm32f103_pmic`)
- `board`, `libraries`, `applications`, `sdk`: legacy RT-Thread build paths kept
  during migration; they remain the source of truth for the current SCons build

The next migration step is to make CMake consume these new directories and the
RT-Thread kernel as a subdirectory, then remove the legacy aliases after a
successful firmware build.
