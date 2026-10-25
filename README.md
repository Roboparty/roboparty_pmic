# RP1.4 背板电源管理芯片

RP1.4 背板电源管理芯片（PMIC）固件项目。MCU 负责采集背板输出电压、与主副 BMS 通信、向 RK3588 SoC 提供 Modbus 数据，并保存运行与故障日志。

## 目标平台

| 项目 | 当前定义 |
| --- | --- |
| MCU | GD32F503，Cortex-M33 |
| RTOS | RT-Thread 5.3.0 主线 |
| 主 BMS | 飞毛腿 21700 电池包，UART 转 RS-485 |
| 副 BMS | 待选型 |
| SoC | RK3588，UART/Modbus |
| 输出采集 | 12 V、24 V、48 V 三路 ADC |
| 外部存储 | SPI Flash |

## 设备资源

- 两路 UART：主 BMS、RK3588 SoC。
- 一路 SPI：外部 Flash。
- 三路 ADC：12 V、24 V、48 V 输出采集。
- RT-Thread 源码：`D:\env\rt-thread-5.3.0`。

## 主 BMS 通信

串口参数为 115200 baud、8N1、高字节优先。

```text
FE FE | CMD | LEN | DATA[LEN] | CRC16_H CRC16_L | BB
```

CRC 覆盖 `CMD + LEN + DATA`，使用 CRC16-CCITT，初值 `0`，多项式 `0x1021`。当前命令为 `0x61` BMS 通用信息查询和 `0x31` 电池信息汇总查询。

## SoC Modbus 通信

MCU 作为 Modbus 从站，地址为 `0x01`，响应 RK3588 的读取请求。

| 数据区 | 起始地址 | 长度 |
| --- | ---: | ---: |
| 主 BMS 通用信息 | `0x00` | 30 字节 |
| 主 BMS 信息汇总 | `0x1E` | 39 字节 |
| 输出电压 | `0x45` | 12 字节 |
| LED 信息 | 待定 | 待定 |

## 故障处理

通信故障采用三帧确认：连续三帧 CRC 错误或超时确认故障，连续三帧正常确认恢复。

| 故障 | 故障行为 | 恢复行为 |
| --- | --- | --- |
| SoC 通信 | 写入 `flash_log`，暂停向 SoC 发送数据 | 记录恢复并恢复通信 |
| 主 BMS 通信 | 保持最后有效数据，禁止相关充电路径 | 恢复数据更新并允许充电 |
| 主 BMS 状态 | 记录故障，按策略禁止充电 | 故障清除后记录恢复 |
| 副 BMS 通信 | 保持最后有效数据，禁止相关充电路径 | 连续三帧正常后恢复 |
| Flash 存储 | 向 SoC 报告存储失败 | 连续三次成功后报告恢复 |
| 输出电压 | 写入 `exception_log` | 回到恢复阈值后记录恢复 |

## 日志与 OTA

SPI Flash 计划保存 `routine_log`、`exception_log`、`MCU_OTA_BIN` 以及主副 BMS 升级镜像。运行日志周期写入，故障事件立即写入。日志字段包括 `ts`、`type`、`level`、`module`、`event`、`msg` 和 `detail`。

系统需要支持量产烧录、MCU OTA、主 BMS OTA、副 BMS OTA、完整性校验和失败回滚。

## 工程结构

```text
boards/roboparty_pmic/       板级启动、时钟、引脚和链接脚本
src/                         PMIC 应用与 RT-Thread 驱动适配
inc/                         项目头文件
thirdparty/                  子模块和厂商 SDK 压缩包
env.ps1                      RT-Thread 和 SDK 环境变量
CMakeLists.txt               构建入口
```

SDK 压缩包：`thirdparty/GD32F50x_Firmware_Library_V1.0.3.7z`。

`roboparty_bms` 子模块：`thirdparty/roboparty_bms`。

## 构建

```powershell
. .\env.ps1
cmake -S . -B build-scud -G Ninja `
  -DCMAKE_C_COMPILER=arm-none-eabi-gcc `
  -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ `
  -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc
cmake --build build-scud
```

当前构建验证生成 `libroboparty_pmic.a`。生成可烧录固件前，还需确认启动文件、时钟、UART 引脚、ADC 分压参数和链接布局。

## 待确认项

- GD32F503 具体封装、Flash/RAM 容量。
- 主副 BMS 型号和 OTA 协议。
- 12 V、24 V、48 V 的 ADC 分压、引脚和采样周期。
- 输出电压过压、欠压和恢复阈值。
- SPI Flash 型号、分区和磨损均衡策略。
- Modbus 完整寄存器表、时间戳同步和 LED 数据格式。
- 量产烧录、签名校验和 OTA 回滚策略。
