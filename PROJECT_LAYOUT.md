# roboparty_pmic project layout

The project follows the Roboparty GD32 USB-CAN layout while retaining the
RT-Thread 5.3.0 BSP build baseline:

- `boards/roboparty_pmic`: board startup, clock, linker and pin definitions
- `drivers`: RT-Thread peripheral driver layer
- `inc`: application and board headers
- `src`: application sources
- `scripts`: project helper scripts
- `thirdparty/GD32F50x_Firmware_Library_V1.0.3`: exact F503 vendor SDK
- `board`, `libraries`, `applications`, `sdk`: legacy RT-Thread build paths kept
  during migration; they remain the source of truth for the current SCons build

The next migration step is to make CMake consume these new directories and the
RT-Thread kernel as a subdirectory, then remove the legacy aliases after a
successful firmware build.
