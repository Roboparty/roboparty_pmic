# STM32F103 PMIC firmware

The current hardware uses one STM32F103C8 and RT-Thread 5.3.0. The bootloader
occupies flash from `0x08000000`; the application starts at `0x08001000`.
The internal `PMIC_LEFT_LAYOUT` name comes from an earlier schematic comparison;
it selects this hardware by default. GD32F503 is a separate MCU target.

## Build

Configure and build the bootloader and application for the current hardware:

```sh
cmake -S boards/stm32f103_pmic/bootloader -B boards/stm32f103_pmic/bootloader/build-current -G Ninja
cmake --build boards/stm32f103_pmic/bootloader/build-current -j 4
cmake -S boards/stm32f103_pmic -B boards/stm32f103_pmic/build/Current -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=boards/stm32f103_pmic/cmake/gcc-arm-none-eabi.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build boards/stm32f103_pmic/build/Current -j 4
```

The outputs are `bootloader/build-current/boot.{bin,hex}` and
`build/Current/ChargeBAT.{bin,hex}`. The CI release archive for future tags is
`stm32f103c8_firmware.tar.gz`; `SHA256SUMS` covers its five images.
The merged factory image is `pmic_stm32f103_v<version>-<YYYYMMDD>.hex`
(for example `pmic_stm32f103_v0.1.12-20261110.hex`); `boot.hex` and
`ChargeBAT.hex` remain available separately. The
already-published `v0.1.7` release uses the historical `-left` asset name.

## Historical Build Name

The earlier side-by-side schematic comparison led to the internal `left` and
`right` names. Only the current STM32F103C8 configuration is the default;
the old alternative remains in the source tree for reference. The current
bootloader accepts USART1 image blocks directly into internal Flash. It does
not probe external SPI flash. Before the serial-OTA engineering
build, the application rejected `OTA_*` commands. Its GPIO and peripheral mapping from the screenshot
is:

| Pins | Current signal | Firmware use |
| --- | --- | --- |
| PA0/PA1/PA2 | I_IN / U_BAT / U_IN | ADC current / battery / input |
| PA3 | ICTRL | TIM2_CH4 charge-current PWM |
| PA4/PA5 | Status LEDs | PA5 green while charging below 54 V battery voltage; PA4 red while charging at or above 54 V; colors verified by SWD GPIO test |
| PA6/PA7 | BAT_G / BAT_R | PA6 high at PA1 = 1.39-1.50 V; PA7 high at PA1 = 1.21-<1.39 V |
| PA8 | FAN | TIM1_CH1 PWM |
| PA9/PA10 | USART1_TX/RX | Host protocol and console (RK3588 UART7) |
| PB3 | EN | Charge enable |
| PB4 | Unconnected | Not configured or read |
| PB8/PB9 | SCL / SDA | Software I2C for CH224 and TMP100 |
| PB10/PB11 | USART3_TX/RX | Not used by the current left build |

PB0-PB2, PB6-PB7, and PB12-PB15 are shown unconnected. OLED,
button, infrared, buzzer, SPI flash, and BMS communication are excluded or inactive in
this variant. The `B` color setting has no physical blue LED output. LED
polarity and the battery-indicator behavior are software assumptions pending
bench observation; the schematic screenshot only identifies their nets.

The application and bootloader both compile. See the hardware test below
for the flashed image and serial checks.

**2026-11-11 change:** the left build routes the host protocol, console, and
OTA to USART1 on PA9/PA10, wired to RK3588 UART7 (`/dev/ttyS7`) at
`10.43.13.56`. The USART3/COM82 statements in the older test logs below
describe the previous wiring.

### USART1 control-port migration (2026-11-11)

- Build: `boot.bin` 2708 B (SHA-256 `af34543a...`), `ChargeBAT.bin` 41860 B
  (SHA-256 `96d68982...`). USART3/PB10/PB11 are unused in the left build.
- Flash: J-Link Pro `10.43.14.82` (S/N 175070461, VTref 3.308 V), full-chip
  erase, `boot.bin` at `0x08000000`, `ChargeBAT.bin` at `0x08001000`;
  VerifyBin passed.
- UART: RK3588 `/dev/ttyS7` at 115200 8N1; HIL 6/6; `SN_GET` 10/10.
  `CHRG_INFO` returned `FW_VER:0.1.10;SW_DATE:2026-11-11;CHG_EN:0;CHG_FAULT:0;`,
  `CHRG_V:5.0V`, `BAT_V:52.8-53.0V`.
- OTA: 41860 bytes, CRC32 `0995A29F`, `OTA_DONE`; J-Link readback matched.
- SN: `066CFF555553787287104026`; UID at `0x1FFFF7E8` is
  `066CFF55 55537872 87104026`.
- BMS forwarding: RK3588 `/etc/default/bms_daemon` has `FORWARD_ENABLE=0`;
  `bms.service` still reads the BMS; no other user holds `/dev/ttyS7`.
- `SW_DATE:2026-11-11;` plus `SAVE:1;` persisted across a J-Link reset.
- Full-chip erase clears the config page; set `SW_DATE:<date>;` and
  `SAVE:1;` after every factory flash.

Before the 2026-11-11 UART1 migration, PB4 and PA9/PA10 were unused. The left
build omitted the BMS driver and thread and did not initialize USART1. In
`0.1.6-left`, the charge
thread automatically enables PB3 EN and sets PA3 ICTRL
after three consecutive ADC samples when input voltage is 16-32 V, CH224
reports nonzero adapter current, and no fault is latched. It limits the
computed ICTRL setting to 70%. Below 16 V, above 32 V, on an ADC failure,
or on a CH224 read failure, it turns charging off. The 55 V battery, 6 A
current, and 80 C temperature thresholds are emergency fault limits, not
normal charge termination; the battery pack's independent cutoff protection
is required. Without a battery-presence input, EN may assert with an adapter
connected even when the pack is absent. Stored `CHG_EN`/`ICTRL` values are
cleared at startup and host writes to those fields are ignored. In the
`0.1.8-left` engineering build, the charge thread owns PA4-PA7; host R/G writes
are ignored. The PA1 indicator thresholds use the ADC pin voltage, not the
scaled battery voltage. At exactly 1.39 V PA6 takes priority. The 54 V
threshold uses the scaled battery/output voltage and only changes the lamp;
it does not terminate charging. The right layout retains its original BMS and
PB4 behavior. On 2026-11-05, SWD GPIO tests showed PA4 drives the red charge
LED and PA5 the green charge LED, correcting the v0.1.7 software mapping.
PA6 drives the battery green LED and PA7 the battery red LED. The
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
read-only cases and the EN register check on 2026-11-05. The corrected
`0.1.8-left` image was flashed and passed 6/6 through A100 on 2026-11-05.
The `0.1.9-left` image was flashed and passed 6/6 on 2026-11-06.

### Fault-field flash and OTA rejection test (2026-11-06)

At 5 V input, no battery connected, `CHG_EN:0` and `CHRG_I:0.0A`, Horco
CMSIS-DAP `3181740232` read the expected chip UID and saved all 64 KiB of
preflash memory. The existing 372-byte bootloader matched SHA-256
`1087d24305d3364376d8bf3f3c455154f514bea1e17a718baa1cdc32d9e99ed5`.
The v0.1.10 Release application was programmed at `0x08001000`; SWD readback
matched its 41,740-byte image exactly (SHA-256
`733c7ed7fa551515f9055b57b19a4600ac53eadf7051d7b2a58b056f1af7325f`).
The bootloader's first 4 KiB were unchanged. After reset, A100 serial HIL
passed 6/6 and reported `FW_VER:0.1.9-left;CHG_FAULT:0;CHG_EN:0;` with
`CHRG_V:5.1V`.

The syntactically valid `OTA_BEGIN:41740,0;` request returned `OTA_ERR;`.
An additional SWD application readback after the request still matched the
Release image exactly, and a further reset still reported `0.1.9-left`.
This STM32 hardware has no serial OTA staging storage. GD32 OTA and loaded
charging were not tested by this operation.

### Direct USART3 OTA (0.1.10 engineering build)

The first installation requires SWD to replace the old jump-only bootloader.
The new bootloader occupies 2,700 of the reserved 4,096 bytes. Application
images remain linked at `0x08001000` and must fit below `0x0800FC00`.
With the new application running, use:

```sh
python3 boards/stm32f103_pmic/tests/stm32_serial_ota.py \
  --port /dev/ttyUSB0 --bin boards/stm32f103_pmic/build/Current/ChargeBAT.bin \
  --uid 0669FF555553787287104058
```

`OTA_ENTER:1;` turns off PB3 EN and resets into the resident bootloader.
`OTA_BEGIN:<size>,<crc32>;` erases the first application page, then
`OTA_DATA:<offset>,<hex>;` streams 64-byte blocks. The bootloader verifies the
Flash contents and CRC32, writes the first page last with the stack pointer
last, and returns `OTA_DONE;`. A reset after an incomplete update leaves the
application vector erased and the bootloader available on USART3. Repeat with
`--recover` to re-send the complete image. The bootloader answers
`OTA_PING:1;` with `OTA_BOOT:1;` in recovery mode.

There is no second image slot and no automatic rollback to the previous
application. Keep SWD available for service recovery. Normal boot waits up to
two seconds for an update command. Do not send updates while a battery is
charging; the board test used a 1.5 V input reading, no battery, and EN=0.

On 2026-11-06, the bootloader was programmed by SWD while the old
application remained byte-for-byte unchanged. A 41,864-byte `0.1.10-left`
application was then transferred over COM82 and returned `OTA_DONE;`; SWD
readback matched the transmitted image byte for byte.
A second update entered from the new application with `OTA_ENTER:1;` and
matched again. An interrupted update left the first vector word as
`0xFFFFFFFF`; `--recover` restored the same full image and version. The
final bootloader also accepted an A100-to-COM82 tunnel update with the
official source build. Its application SWD readback matched SHA-256
`1cf770aa843c32d97810fd28e2eff16b08b8a7573fcc03d468491f861ac1a024`.
The post-update board reported `FW_VER:0.1.10-left;CHG_FAULT:0;CHG_EN:0;`
and passed 6/6 read-only HIL checks both locally and from A100.
The version suffix was then removed. A100 serial OTA transferred the 41,860-byte
`0.1.10` application; SWD readback matched SHA-256
`edbb3b3a388e2ddaf13281cabaa61c8900eedf1edd50411da00d59a03fc92160`.
After reset, `FW_VER:0.1.10;` and the A100 read-only HIL passed 6/6.

After the user restored 20 V PD and confirmed no battery was connected, the
read-only serial HIL again passed 6/6. `CHRG_INFO` reported 19.9-20.0 V input,
54.3-54.6 V open-circuit output, 0.0-0.1 A indicated current,
`CHG_FAULT:0;CHG_EN:1;`. The user reported the indicator lights looked normal.
This open-circuit check does not establish charging current or termination.

### Current hardware test (2026-11-05)

The Horco CMSIS-DAP `3181740232` confirmed the same STM32F103C8 UID, and
the preflash 64 KiB image was saved. The `0.1.8-left` application was written
at `0x08001000`, read back byte for byte (SHA-256
`a3d6131448d67cc81f71fe9e5819ed3885ee58ea284fc567f558157b6ab8236f`),
and reset. The bootloader remained unchanged. A100 `22282` read-only HIL
passed 6/6 and reported `FW_VER:0.1.8-left;`.

With no battery connected, a 5 V Type-C source produced `CHG_V:5.0V`,
`CHG_EN:0`, `ICTRL:0`, and PA4/PA5 low. The 20 V PD source produced
`CHRG_V:19.8-19.9V`, `BAT_V:54.6-54.7V`, `CHG_EN:1`, `ICTRL:70`, PA4 high,
and PA5 low; the user visually confirmed only the charge red LED was on.
SWD halted the MCU at 5 V input and separately drove each output high:
PA4 lit the charge red LED, PA5 the charge green LED, PA6 the battery green
LED, and PA7 the battery red LED. The MCU was reset afterward. Host R/G,
CHG_EN, and ICTRL writes did not override the automatic state. PA1 was not
injected because it could not be connected safely; its 1.21, 1.39, and
1.50 V boundaries passed offline policy tests only. The green charging state
below 54 V was not reached with the open-circuit output. No loaded charging
or charge termination was tested.

### Identity queries

`SN_GET:1;` returns `CHRG_SN:<24 hex digits>;` from the STM32F103's 96-bit
unique device ID at `UID_BASE` (`0x1FFFF7E8`). The three 32-bit words are
printed in ascending address order, with eight uppercase hex digits each.
`CHRG_INFO:1;` includes the same SN. `CHRG_SN:<value>;` now replies
`SN_READ_ONLY;` on STM32F103 and cannot change this hardware ID.
`FW_VER:1;` returns the compiled firmware identifier
`FW_VER:0.1.10;` in the current source build. The `0.1.8-left` image
was flashed and verified on 2026-11-05; the old alternative build uses
`0.1.2-right`.
`SW_DATE` is separate writable metadata, not the firmware version.

### Host serial protocol

The host port is USART1 on PA9 (TX) and PA10 (RX), 115200 baud, 8 data
bits, no parity, one stop bit. It is wired to RK3588 UART7 (`/dev/ttyS7`) at
`10.43.13.56`. The earlier DAPLink COM82-to-A100 `22282` bridge is no longer
the control path. Commands are ASCII
`NAME:DATA;`; replies are ASCII `FIELD:value;`, with several fields
concatenated for `CHRG_INFO` and `SYS_INFO`. No newline or checksum is used
for ordinary commands. The parser has a 512-byte frame, but the RT-Thread
serial RX ring is 64 bytes: send short commands and wait for each reply. It
acts on `;` or 50 ms of input silence. Setting commands have no success ACK.

| Request | Reply or action |
| --- | --- |
| `SN_GET:1;` | `CHRG_SN:<96-bit UID as 24 uppercase hex digits>;` |
| `FW_VER:1;` | `FW_VER:0.1.10;` in the current source build |
| `CHRG_INFO:1;` | SN, date, version, config, input/battery readings, `CHG_FAULT:0/1;`, charge status |
| `SYS_INFO:1;` | uptime and thread stack high-water marks |
| `BAT_INST:1;`, `IR_REC:1;` | On left both return 0 as legacy placeholders; neither input is connected |
| `BAT_SOC:1;`, `BAT_SOH:1;` | On left return `BMS_DISABLED;`; right layout reads cached BMS values |
| `R:n;`, `G:n;`, `B:n;`, `FAN:n;` | Set 0-100 in RAM; no ACK |
| `ICTRL:n;`, `CHG_EN:n;` | Ignored on left; right layout retains writable settings, subject to charge logic |
| `SW_DATE:text;`, `SAVE:1;` | Set date in RAM; explicitly save config to internal Flash |
| `CHRG_SN:text;` | `SN_READ_ONLY;` on STM32F103 |
| `OTA_ENTER:1;` | `OTA_BOOT;` then reset to USART1 bootloader |
| `OTA_BEGIN/DATA/END` | Bootloader image transfer; see Direct USART3 OTA above for the earlier wiring |

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
