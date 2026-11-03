/**
 * @file modbus_bms_regs.h
 * @brief BMS Modbus 寄存器地址定义（基于从站地址0x01，寄存器起始地址0x9000）
 * @note 所有地址均为16位寄存器地址，每个寄存器占2字节（大端模式）
 *       读取/写入时使用已实现的 Modbus_ReadHoldingRegisters / Modbus_WriteSingleRegister 函数
 */

#ifndef TWS_BMS_H
#define TWS_BMS_H

#include <stdint.h>

/* 错误码 */
#define MODBUS_OK               0x00
#define MODBUS_ERR_TIMEOUT      0x01    /* 接收超时 */
#define MODBUS_ERR_CRC          0x02    /* CRC 校验错误 */
#define MODBUS_ERR_SLAVE_ADDR   0x03    /* 从机地址不匹配 */
#define MODBUS_ERR_FUNC_CODE    0x04    /* 功能码不匹配 */
#define MODBUS_ERR_FRAME_LEN    0x05    /* 帧长度错误 */
#define MODBUS_ERR_TX_FAIL      0x06    /* 发送失败 */
#define MODBUS_ERR_PARAM        0x07    /* 参数错误（寄存器数量为 0） */

/* 接收超时时间（毫秒），等待从机应答 */
#define RX_TIMEOUT_MS           1000


/*==============================================================================
 * 寄存器地址定义（绝对地址）
 * 范围：0x9000 ~ 0x902E 及后续扩展（reserve5占用至0x9035）
 *============================================================================*/

/* 系统状态 */
#define REG_EMS_WORK_STATE      0x9000U     /* uint16_t, EMS工作状态: 0=IDLE,1=CHARGE,2=DISCHARGE,3=SLEEP,4=PROTECT,5=SHUTDOWN */
#define REG_RESERVE_9001        0x9001U     /* uint16_t, 保留 */

/* 电池基本参数 */
#define REG_BAT_VOLTAGE         0x9002U     /* uint32_t, 电池包总电压 (单位: mV), 占用 0x9002(高16位) + 0x9003(低16位) */
#define REG_CURRENT             0x9004U     /* uint32_t, 总电流 (单位: mA), 占用 0x9004 + 0x9005 */

/* 电压极值 */
#define REG_MAX_CELL_VOL        0x9006U     /* uint16_t, 最高单体电压 (mV) */
#define REG_MIN_CELL_VOL        0x9007U     /* uint16_t, 最低单体电压 (mV) */

/* 温度（注意：原始值需减去40得到实际摄氏度） */
#define REG_MAX_CELL_TEMP       0x9008U     /* uint16_t, 最高单体温度, 实际温度 = 原始值 - 40 (°C) */
#define REG_MIN_CELL_TEMP       0x9009U     /* uint16_t, 最低单体温度, 实际温度 = 原始值 - 40 (°C) */
#define REG_MAX_BMS_BOARD_TEMP  0x900AU     /* uint16_t, 最高BMS板温度, 实际温度 = 原始值 - 40 (°C) */
#define REG_MIN_BMS_BOARD_TEMP  0x900BU     /* uint16_t, 最低BMS板温度, 实际温度 = 原始值 - 40 (°C) */

/* 保护与IO状态 */
#define REG_PROTECT_STATUS      0x900CU     /* uint32_t, 保护状态位, 占用 0x900C + 0x900D */
#define REG_RESERVE2            0x900EU     /* uint32_t, 保留2, 占用 0x900E + 0x900F */
#define REG_RESERVE3            0x9010U     /* uint32_t, 保留3, 占用 0x9010 + 0x9011 */
#define REG_RESERVE4            0x9012U     /* uint32_t, 保留4, 占用 0x9012 + 0x9013 */
#define REG_IO_STATUS           0x9014U     /* uint32_t, IO状态:
                                               bit0: 充电MOS控制, bit1: 放电MOS控制,
                                               bit2: 充电MOS状态, bit3: 放电MOS状态 */

/* 产品标识 */
#define REG_SN_START            0x9016U     /* ASCII 序列号, 共32字节, 占用16个寄存器 (0x9016 ~ 0x9025) */

/* 版本信息 */
#define REG_BCU_SW_VERSION      0x9026U     /* uint16_t, 软件版本, 例 0x0101 = V1.01 */
#define REG_BCU_HD_VERSION      0x9027U     /* uint16_t, 硬件版本, 例 0x0101 = V1.01 */

/* 电量与寿命 */
#define REG_BMS_SOC             0x9028U     /* uint16_t, 荷电状态 (单位: 1%) */
#define REG_BMS_SOH             0x9029U     /* uint16_t, 健康状态 (单位: 1%) */
#define REG_REMAIN_CAPACITY     0x902AU     /* uint32_t, 剩余容量 (单位: mAh), 占用 0x902A + 0x902B */
#define REG_CYCLES              0x902CU     /* uint32_t, 放电循环次数, 占用 0x902C + 0x902D */

/* 保留区域 */
#define REG_RESERVE5_START      0x902EU     /* 保留5起始地址, 共16字节 (8个寄存器), 实际占用 0x902E ~ 0x9035 */

/*==============================================================================
 * 辅助宏：计算寄存器数量（用于批量读取）
 *============================================================================*/
#define REG_COUNT_EMS_WORK_STATE        1   /* 0x9000 */
#define REG_COUNT_BAT_VOLTAGE           2   /* 0x9002-0x9003 */
#define REG_COUNT_CURRENT               2   /* 0x9004-0x9005 */
#define REG_COUNT_MAX_CELL_VOL          1
#define REG_COUNT_MIN_CELL_VOL          1
#define REG_COUNT_MAX_CELL_TEMP         1
#define REG_COUNT_MIN_CELL_TEMP         1
#define REG_COUNT_MAX_BMS_BOARD_TEMP    1
#define REG_COUNT_MIN_BMS_BOARD_TEMP    1
#define REG_COUNT_PROTECT_STATUS        2   /* 0x900C-0x900D */
#define REG_COUNT_IO_STATUS             2   /* 0x9014-0x9015 */
#define REG_COUNT_SN                    16  /* 0x9016-0x9025, 32字节ASCII */
#define REG_COUNT_BCU_SW_VERSION        1
#define REG_COUNT_BCU_HD_VERSION        1
#define REG_COUNT_BMS_SOC               1
#define REG_COUNT_BMS_SOH               1
#define REG_COUNT_REMAIN_CAPACITY       2
#define REG_COUNT_CYCLES                2
#define REG_COUNT_RESERVE5              8   /* 0x902E-0x9035, 16字节 */

/*==============================================================================
 * 可选：结构体映射（用于直接映射连续寄存器区域，注意字节序需与设备一致）
 * 注意：此结构体假设寄存器地址连续且无空洞，实际使用时应谨慎对齐和大小端
 *============================================================================*/
#pragma pack(push, 1)
typedef struct {
    uint16_t ems_work_state;        /* 0x9000 */
    uint16_t reserve_9001;          /* 0x9001 */
    uint32_t bat_voltage;           /* 0x9002-0x9003 */
    uint32_t current;               /* 0x9004-0x9005 */
    uint16_t max_cell_vol;          /* 0x9006 */
    uint16_t min_cell_vol;          /* 0x9007 */
    uint16_t max_cell_temp;         /* 0x9008 */
    uint16_t min_cell_temp;         /* 0x9009 */
    uint16_t max_bms_board_temp;    /* 0x900A */
    uint16_t min_bms_board_temp;    /* 0x900B */
    uint32_t protect_status;        /* 0x900C-0x900D */
    uint32_t reserve2;              /* 0x900E-0x900F */
    uint32_t reserve3;              /* 0x9010-0x9011 */
    uint32_t reserve4;              /* 0x9012-0x9013 */
    uint32_t io_status;             /* 0x9014-0x9015 */
    uint8_t  sn[32];                /* 0x9016-0x9025, 注意：Modbus寄存器为大端，sn实际存储为ASCII字符，每寄存器两个字符 */
    uint16_t bcu_sw_version;        /* 0x9026 */
    uint16_t bcu_hd_version;        /* 0x9027 */
    uint16_t bms_soc;               /* 0x9028 */
    uint16_t bms_soh;               /* 0x9029 */
    uint32_t remain_capacity;       /* 0x902A-0x902B */
    uint32_t cycles;                /* 0x902C-0x902D */
    uint8_t  reserve5[16];          /* 0x902E-0x9035 */
} bms_register_map_t;
#pragma pack(pop)

/* 编译时检查结构体大小是否与寄存器范围匹配（可选） */
_Static_assert(sizeof(bms_register_map_t) == (0x9035 - 0x9000 + 1) * 2,
               "BMS register map size mismatch");

extern uint8_t Modbus_ReadHoldingRegisters(uint8_t slave_addr, uint16_t start_reg, uint16_t reg_count, uint16_t *dest_buffer, uint16_t recv_len);
extern uint8_t Modbus_WriteSingleRegister(uint8_t slave_addr, uint16_t reg_addr, uint16_t value);

#endif /* MODBUS_BMS_REGS_H */