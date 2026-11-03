# STM32F103 PMIC firmware

This board target uses RT-Thread 5.3.0 and an STM32F103C8. The bootloader
occupies flash from `0x08000000`; the application starts at `0x08001000`.

## Build

Build the bootloader and application with their existing CMake build trees:

```sh
cmake --build boards/stm32f103_pmic/bootloader/build -j 4
cmake --build boards/stm32f103_pmic/build/Release -j 4
```

The outputs are `bootloader/build/boot.{bin,hex}` and
`build/Release/ChargeBAT.{bin,hex}`. The release archive is
`build/stm32f103_ota_images.tar.gz`; `SHA256SUMS` covers its four images.

## Left-schematic variant

The side-by-side MCU screenshot supplied on 2026-11-03 shows a second board
layout. Build it in separate directories; do not flash these images to the
right-schematic board:

```sh
cmake -S boards/stm32f103_pmic -B boards/stm32f103_pmic/build/Left -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=boards/stm32f103_pmic/cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Release -DPMIC_LEFT_LAYOUT=ON
cmake --build boards/stm32f103_pmic/build/Left -j 4
cmake -S boards/stm32f103_pmic/bootloader \
  -B boards/stm32f103_pmic/bootloader/build-left -G Ninja -DPMIC_LEFT_LAYOUT=ON
cmake --build boards/stm32f103_pmic/bootloader/build-left -j 4
```

The left bootloader jumps directly to the application at `0x08001000`; it
does not probe external SPI flash. The left application rejects `OTA_*`
commands with `OTA_ERR;`. Its GPIO and peripheral mapping from the screenshot
is:

| Pins | Left-schematic signal | Firmware use |
| --- | --- | --- |
| PA0/PA1/PA2 | I_IN / U_BAT / U_IN | ADC current / battery / input |
| PA3 | ICTRL | TIM2_CH4 charge-current PWM |
| PA4/PA5 | LED_R / LED_G | Digital status LEDs; nonzero R/G means on |
| PA6/PA7 | BAT_G / BAT_R | Battery present and healthy / fault indicators |
| PA8 | FAN | TIM1_CH1 PWM |
| PA9/PA10 | USART1_TX/RX | BMS serial |
| PB3/PB4 | EN / BATINS | Charge enable / battery detect |
| PB8/PB9 | SCL / SDA | Software I2C for CH224 and TMP100 |
| PB10/PB11 | USART3_TX/RX | Host protocol and console |

PB0-PB2, PB6-PB7, and PB12-PB15 are shown unconnected on the left. OLED,
button, infrared, buzzer, SPI flash, and OTA code are excluded or inactive in
this variant. The `B` color setting has no physical blue LED output. LED
polarity and the battery-indicator behavior are software assumptions pending
bench observation; the schematic screenshot only identifies their nets.

The left application and bootloader both compile. See the hardware test below
for the flashed image and serial checks.

### Identity queries

`SN_GET:1;` returns `CHRG_SN:<24 hex digits>;` from the STM32F103's 96-bit
unique device ID at `UID_BASE` (`0x1FFFF7E8`). The three 32-bit words are
printed in ascending address order, with eight uppercase hex digits each.
`CHRG_INFO:1;` includes the same SN. `CHRG_SN:<value>;` now replies
`SN_READ_ONLY;` on STM32F103 and cannot change this hardware ID.
`FW_VER:1;` returns the compiled firmware identifier
`FW_VER:0.1.2-left;`; the right-schematic variant uses `0.1.2-right`.
`SW_DATE` is separate writable metadata, not the firmware version.

### Host serial protocol

The host port is USART3 on PB10 (TX) and PB11 (RX), 115200 baud, 8 data
bits, no parity, one stop bit. The DAPLink CDC port is COM82 on Windows and
is forwarded to `127.0.0.1:22282` on A100. Commands are ASCII
`NAME:DATA;`; replies are ASCII `FIELD:value;`, with several fields
concatenated for `CHRG_INFO` and `SYS_INFO`. No newline or checksum is used
for ordinary commands. The parser has a 512-byte frame, but the RT-Thread
serial RX ring is 64 bytes: send short commands and wait for each reply. It
acts on `;` or 50 ms of input silence. Setting commands have no success ACK.

| Request | Reply or action |
| --- | --- |
| `SN_GET:1;` | `CHRG_SN:<96-bit UID as 24 uppercase hex digits>;` |
| `FW_VER:1;` | `FW_VER:0.1.2-left;` |
| `CHRG_INFO:1;` | SN, date, version, config, input/battery readings, charge status |
| `SYS_INFO:1;` | uptime and thread stack high-water marks |
| `BAT_INST:1;`, `IR_REC:1;` | Detection fields; IR is absent on left layout |
| `BAT_SOC:1;`, `BAT_SOH:1;` | Cached BMS values; BMS is polled only with battery present |
| `R:n;`, `G:n;`, `B:n;`, `FAN:n;`, `ICTRL:n;` | Set 0-100 in RAM; no ACK |
| `CHG_EN:n;` | Set 0 or 1 in RAM; charge logic may override it |
| `SW_DATE:text;`, `SAVE:1;` | Set date in RAM; explicitly save config to internal Flash |
| `CHRG_SN:text;` | `SN_READ_ONLY;` on STM32F103 |
| `OTA_*:...;` | `OTA_ERR;` on the left layout (no external SPI Flash) |

The `B` setting is retained for protocol compatibility but has no physical
blue LED on the left layout. An empty `SW_DATE` is expected until set and
saved. Send one request at a time and wait for its terminating `;` fields;
short TCP sessions through the serial bridge can lose a single request.

### Left-board hardware test (2026-11-03)

The Horco CMSIS-DAP probe `3181740232` programmed the left bootloader and
application with GDB `load`; `compare-sections` matched every loaded section.
Before flashing, the 64 KiB target Flash was saved as
`build/Left/preflash_20261103.bin` (SHA256
`d541b65ce41d1781f869dc5904e1177624dbde5564cb74363bf4d20e6ffdbb17`).
The configuration page at `0x0800FC00` was blank before and after flashing.

The initial 0.1.0-left image returned `FW_VER:0.1.0-left;`, an empty stored
`CHRG_SN`, a complete `CHRG_INFO`, and a complete `SYS_INFO`. `CHRG_INFO`
reported `BAT_INST:0`, `ICTRL:0`, `CHG_EN:0`, and
`CHRG_I:0.0A`. A 60-second A100-to-22282 soak completed 15/15 `SYS_INFO`
and 15/15 `CHRG_INFO` requests with no empty replies or errors; uptime rose
from 108,765 to 164,832 ms. Maximum measured stack use was CHARGE 184/1024,
BMS 160/1024, and HOST 1816/2560 bytes.

Single short-lived A100 connections for `SN_GET` or `OTA_BEGIN` sometimes
received no reply. Repeating the command in one connection returned
`CHRG_SN:;` and `OTA_ERR;`, respectively. This is a remaining serial-bridge
reliability issue; the local COM82 query and 60-second alternating-query
test succeeded. With no battery detected, charging regulation and BMS traffic
could not be exercised. LED/fan polarity and physical output still need
direct bench observation.

The later 0.1.2-left image was programmed with GDB and passed
`compare-sections`. pyOCD read the UID words at `0x1FFFF7E8` as
`0669FF55 55537872 87104058`. The serial replies matched exactly:
`CHRG_SN:0669FF555553787287104058;` and `FW_VER:0.1.2-left;`.
`CHRG_SN:TEST;` returned `SN_READ_ONLY;`; a subsequent `CHRG_INFO` still
reported the same UID and `CHG_EN:0`. `BAT_INST` and `IR_REC` each returned
`0` with a terminating semicolon.

## Startup requirements

- `RT_USING_SMALL_MEM_AS_HEAP` is required with `RT_USING_SMALL_MEM`. Without
  it, RT-Thread's system `rt_malloc()` returns `NULL` and main-thread startup
  faults.
- The main thread uses a 2048-byte stack, the host thread uses a 2560-byte
  stack, the UI thread uses a 1280-byte stack, and the system heap is 8192
  bytes.
- `stdout` is unbuffered because host replies end with `;` rather than a
  newline. Stack diagnostics scan RT-Thread's `'#'` fill bytes.
- The W25Q128 probe has a finite busy timeout. When the external flash is
  absent or unresponsive, the bootloader jumps to the installed application.

## Hardware check

On 2026-11-03, the bootloader and application were programmed through
CMSIS-DAP and GDB, then `compare-sections` matched each flash section. The
RT-Thread tick advanced and the `host` thread was present after reset. Three
consecutive local `SYS_INFO:1;` requests over DAPLink COM82 returned complete
replies with increasing uptime. An A100 request through the remote `22282`
serial bridge also returned a complete reply.

## Read-only functional and stability test (2026-11-03)

From A100, run:

```sh
python3 boards/stm32f103_pmic/tests/pmic_soak.py --duration 180 --interval 2
```

Keep the Windows COM82-to-22282 bridge running. The test
opens one TCP connection per request and alternates `SYS_INFO:1;` and
`CHRG_INFO:1;`.

The 180-second run returned 45/45 complete `SYS_INFO` replies and 45/45
complete `CHRG_INFO` replies, with zero empty replies or socket errors.
UPTIME increased from 3,337,214 to 3,513,419 ms without a reset. Maximum
reported stack use was CHARGE 184/1024, BMS 160/1024, UI 856/1280, and
HOST 1864/2560 bytes. `CHRG_INFO` consistently reported `BAT_INST:0`,
`CHG_EN:0`, `ICTRL:0`, `CHRG_I:0.0A`, `CHRG_V:1.6V`, and `BAT_V:3.6V`.
The latter two readings are unverified sensor readings without a battery.

Read-only `BAT_INST`, `IR_REC`, and `BAT_SOC` requests returned 0. A single
`BAT_SOH` request sometimes returned no data; sending it twice in one
connection returned `BMS_SOH:0;`. This short-command behavior remains to be
investigated. With `BAT_INST:0`, the firmware does not poll the BMS, so BMS
communication and real charging regulation were not exercised. OLED pixels,
key presses, fan, buzzer, and charging power output need direct observation or
an instrumented bench setup. No output-setting commands were sent in this
read-only test.
