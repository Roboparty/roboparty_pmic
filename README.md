# roboparty_pmic

GD32F503 + RT-Thread 5.3.0 firmware project.

## Status

This project starts from the RT-Thread `gd32e503v-eval` BSP layout and uses the
GD32F5xx vendor HAL in `libraries/gd32f5xx`. The board-level pin map, clock,
startup file, linker layout, and peripheral validation must be checked against
the actual GD32F503 package and PMIC board schematic before flashing hardware.

## Build

```powershell
cd D:\GitHub\roboparty_pmic
scons -j4
```

Use the RT-Thread 5.3.0 environment and the ARM GNU toolchain. The current
skeleton keeps the RT-Thread console/UART application and is intended as the
first compile/bring-up baseline.
