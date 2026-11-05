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
| PA6/PA7 | BAT_G / BAT_R | BAT_G stays off without BMS; BAT_R shows a latched exception |
| PA8 | FAN | TIM1_CH1 PWM |
| PA9/PA10 | No connected 485/BMS interface | Not configured or used on the left layout |
| PB3 | EN | Charge enable |
| PB4 | Unconnected | Not configured or read on the left layout |
| PB8/PB9 | SCL / SDA | Software I2C for CH224 and TMP100 |
| PB10/PB11 | USART3_TX/RX | Host protocol and console |

PB0-PB2, PB6-PB7, and PB12-PB15 are shown unconnected on the left. OLED,
button, infrared, buzzer, SPI flash, BMS communication, and OTA code are excluded or inactive in
this variant. The `B` color setting has no physical blue LED output. LED
polarity and the battery-indicator behavior are software assumptions pending
bench observation; the schematic screenshot only identifies their nets.

The left application and bootloader both compile. See the hardware test below
for the flashed image and serial checks.

Since `0.1.4-left`, PB4 and PA9/PA10 are unused. The left build omits the BMS
driver and thread and does not initialize USART1. In `0.1.6-left`, the charge
thread automatically enables PB3 EN and sets PA3 ICTRL
after three consecutive ADC samples when input voltage is 16-32 V, CH224
reports nonzero adapter current, and no fault is latched. It limits the
computed ICTRL setting to 70%. Below 16 V, above 32 V, on an ADC failure,
or on a CH224 read failure, it turns charging off. The 55 V battery, 6 A
current, and 80 C temperature thresholds are emergency fault limits, not
normal charge termination; the battery pack's independent cutoff protection
is required. Without a battery-presence input, EN may assert with an adapter
connected even when the pack is absent. Stored `CHG_EN`/`ICTRL` values are
cleared at startup and host writes to those fields are ignored. BAT_G stays
off because no battery state is available. The right layout retains its
original BMS and PB4 behavior. `0.1.6-left` changes only the version string
from the tested `0.1.5-left` behavior; it has not been flashed. The
`0.1.5-left` hardware results are recorded below.

The host-command guard has a separate offline test. Its host build suppresses
an existing signedness warning in the unrelated Flash-save function:

```sh
gcc -std=c11 -Wall -Wextra -Werror -Wno-error=sign-compare \
  -DPMIC_LEFT_LAYOUT -ffunction-sections -fdata-sections \
  -Iboards/stm32f103_pmic/tests/charge_guard -Icommon/app/inc \
  -Icommon/bsp -Iboards/stm32f103_pmic \
  boards/stm32f103_pmic/tests/charge_guard/test_charge_commands.c \
  common/app/src/app_config.c -Wl,--gc-sections \
  -o ../../test/tmp/pmic-charge-commands
../../test/tmp/pmic-charge-commands
gcc -std=c11 -Wall -Wextra -Werror -Icommon/app/inc \
  boards/stm32f103_pmic/tests/charge_guard/test_auto_enable.c \
  -o ../../test/tmp/pmic-auto-enable
../../test/tmp/pmic-auto-enable
```

Use `python3 boards/stm32f103_pmic/tests/hil_charger.py --layout left --self-test`
for the offline left-layout response contract. The real-port test passed on
COM82 and through A100 `socket://127.0.0.1:22282` for the flashed
`0.1.4-left` image on 2026-11-04. The `0.1.5-left` image passed the COM82
read-only cases and the EN register check on 2026-11-05.

### Identity queries

`SN_GET:1;` returns `CHRG_SN:<24 hex digits>;` from the STM32F103's 96-bit
unique device ID at `UID_BASE` (`0x1FFFF7E8`). The three 32-bit words are
printed in ascending address order, with eight uppercase hex digits each.
`CHRG_INFO:1;` includes the same SN. `CHRG_SN:<value>;` now replies
`SN_READ_ONLY;` on STM32F103 and cannot change this hardware ID.
`FW_VER:1;` returns the compiled firmware identifier
`FW_VER:0.1.6-left;` in the release build. The test board was last flashed
with `0.1.5-left`; the right-schematic variant uses `0.1.2-right`.
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
| `FW_VER:1;` | `FW_VER:0.1.6-left;` in this release build |
| `CHRG_INFO:1;` | SN, date, version, config, input/battery readings, charge status |
| `SYS_INFO:1;` | uptime and thread stack high-water marks |
| `BAT_INST:1;`, `IR_REC:1;` | On left both return 0 as legacy placeholders; neither input is connected |
| `BAT_SOC:1;`, `BAT_SOH:1;` | On left return `BMS_DISABLED;`; right layout reads cached BMS values |
| `R:n;`, `G:n;`, `B:n;`, `FAN:n;` | Set 0-100 in RAM; no ACK |
| `ICTRL:n;`, `CHG_EN:n;` | Ignored on left; right layout retains writable settings, subject to charge logic |
| `SW_DATE:text;`, `SAVE:1;` | Set date in RAM; explicitly save config to internal Flash |
| `CHRG_SN:text;` | `SN_READ_ONLY;` on STM32F103 |
| `OTA_*:...;` | `OTA_ERR;` on the left layout (no external SPI Flash) |

The `B` setting is retained for protocol compatibility but has no physical
blue LED on the left layout. An empty `SW_DATE` is expected until set and
saved. Send one request at a time and wait for its terminating `;` fields;
short TCP sessions through the serial bridge can lose a single request.

### Left-board 0.1.5 build (2026-11-05)

The automatic-enable policy and host-command guard tests passed on A100.
The left application compiled and linked at 41,436 bytes Flash / 16,096
bytes RAM. Its `ChargeBAT.bin` SHA-256 is
`44b8e871253a8c835983d2a74f818100efa88a6d888e7deb9c53720e91f00773`.
The right-layout application was rebuilt with the unchanged SHA-256
`0ec303c48d95daaa94d04e94523528e9cc05f79b0e177f9a86e924903349878f`.
Horco CMSIS-DAP `3181740232` matched UID `0669FF55 55537872 87104058`.
The prior application was backed up and matched the `0.1.4-left` SHA-256.
pyOCD programmed only the application at `0x08001000`; the 41,436-byte
readback matched `0.1.5-left` byte for byte, and the config page remained
erased. The probe then reset and resumed the target. COM82 returned the new
version and matching UID; the read-only HIL test passed 6/6.

With a 5 V-only Type-C source, `CHRG_V` was 5.1 V, `CHG_EN` and `ICTRL` were
0, PB3 output was low, and TIM2 CCR4 was 0. After switching to a PD source,
`CHRG_V` was 19.8-20.0 V, `CHG_EN` was 1, `ICTRL` was 70, PB3 output was high,
and TIM2 CCR4 was 5040. Ten status reads over about 20 seconds were complete;
`BAT_V` was 54.5-54.8 V and indicated current was 0.0-0.1 A. The battery pack
was not connected, so this was an open-circuit output test. These are MCU
reported values; external EN voltage, actual charge current, temperature,
and charge termination have not been measured.

### Left-board 0.1.4 hardware test (2026-11-04)

Horco CMSIS-DAP `3181740232` read UID words `0669FF55 55537872 87104058`,
matching the previous board record. The full 64 KiB Flash was saved before
programming. pyOCD wrote only the application at `0x08001000` with sector
erase; a 41,156-byte readback matched the binary byte for byte (SHA-256
`9d8354091d111549d6f6ac054b0c52d229175af73bee12f572207f2cfb4a9224`).
The configuration page at `0x0800FC00` remained erased. The probe reset and
resumed the MCU after programming.

COM82 returned `FW_VER:0.1.4-left;` and
`CHRG_SN:0669FF555553787287104058;`. The left-layout HIL read-only cases
passed 6/6 on COM82, then 10/10 repeated runs, and 6/6 through A100 port
`22282`. `BAT_SOC` and `BAT_SOH` returned `BMS_DISABLED;`, and `SYS_INFO`
reported CHARGE and HOST stacks without a BMS stack. After sending
`CHG_EN:1;` and `ICTRL:100;`, `CHRG_INFO` still reported `CHG_EN:0` and
`ICTRL:0`. Debug-register reads showed PB3 output bit 0 and TIM2 CCR4 0.

The 60-second A100 read-only soak returned 15/15 complete `SYS_INFO` and
15/15 complete `CHRG_INFO` responses with zero empty replies or errors.
Uptime rose monotonically from 414,362 to 470,430 ms. Maximum observed
stack use was CHARGE 176/1024 and HOST 1824/2560 bytes. This validates the
software output state, but no external voltage, current, LED, fan, PD, or
loaded charging measurement was made.

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
