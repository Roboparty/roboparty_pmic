
// SPDX-License-Identifier: GPL-3.0
// Copyright (C) 2026 zz020


/**
 * @file modbus_master.c
 * @brief Modbus RTU 主机接口，用于读取 BMS 寄存器
 * @note 使用 USART1 与 BMS 通信
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <rtdevice.h>
#include "tws_bms.h"
#include "bms.h"
#include <rtthread.h>
#include "bsp.h"

/* 底层串口收发接口（由 bms_send/bms_receive 实现） */
//extern bool UART_Transmit(const uint8_t *data, uint16_t len);      /* 发送数据，成功返回 true */
//extern uint16_t UART_Receive(uint8_t *buffer, uint16_t max_len, uint32_t timeout_ms); /* 接收数据，返回实际长度 */

/* CRC 计算函数（见文件末尾） */
uint16_t modbus_crc16(const uint8_t *pucFrame, uint16_t usLen);
static rt_err_t bms_receive(uint8_t *buffer, uint16_t length, uint32_t timeout_ms);
static int bms_send(const uint8_t *data, uint16_t length);

/**
 * @brief 读取保持寄存器（功能码 0x03）
 * @param slave_addr     从机地址（1~247）
 * @param start_reg      起始寄存器地址（16 位）
 * @param reg_count      寄存器数量（最大 125，对应数据区 <=250 字节）
 * @param dest_buffer    接收缓冲区（至少 reg_count * 2 字节）
 * @return               0 成功，其他为错误码
 */
uint8_t Modbus_ReadHoldingRegisters(uint8_t slave_addr, uint16_t start_reg,
                                    uint16_t reg_count, uint16_t *dest_buffer, uint16_t recv_len)
{
    uint8_t request[8];      /* 请求帧固定 8 字节 */
    uint8_t response[260];   /* 响应帧：地址+功能码+字节数+250 字节数据+2 字节 CRC = 254，留余量 */
    uint16_t crc;
    uint8_t *ptr;
    uint8_t byte_count;
    uint16_t i;
    uint16_t calc_crc;

    /* 参数检查 */
    if (dest_buffer == NULL || reg_count == 0 || reg_count > 125) {
        return MODBUS_ERR_PARAM;
    }

    /* 组帧 */
    request[0] = slave_addr;                     /* 从机地址 */
    request[1] = 0x03;                           /* 功能码：读保持寄存器 */
    request[2] = (uint8_t)(start_reg >> 8);      /* 起始地址高字节 */
    request[3] = (uint8_t)(start_reg & 0xFF);    /* 起始地址低字节 */
    request[4] = (uint8_t)(reg_count >> 8);      /* 寄存器数量高字节 */
    request[5] = (uint8_t)(reg_count & 0xFF);    /* 寄存器数量低字节 */
    crc = modbus_crc16(request, 6);              /* 计算 CRC（前 6 字节） */
    request[6] = (uint8_t)(crc & 0xFF);          /* CRC 低字节 */
    request[7] = (uint8_t)(crc >> 8);            /* CRC 高字节 */

    if (bms_send(request, 8) != 8) {
        printf("request send TIMEOUT;");
        return MODBUS_ERR_TX_FAIL;
    }

    uint16_t frame_len = 3 + recv_len + 2;

    if (bms_receive(response, frame_len, RX_TIMEOUT_MS) != RT_EOK) {
        return MODBUS_ERR_TIMEOUT;
    }

    if (response[0] != slave_addr) {
        printf("BMS Slave Address NOT MATCH: 0x%X;", response[0]);
        return MODBUS_ERR_SLAVE_ADDR;
    }
    if (response[1] != 0x03) {
        printf("BMS Response CMD NOT MATCH: 0x%X;", response[1]);
        return MODBUS_ERR_FUNC_CODE;
    }

    byte_count = response[2];
    if (byte_count != (reg_count * 2)) {
        printf("BMS Read PLD Len Mismatch;\n");
        return MODBUS_ERR_FRAME_LEN;
    }

    calc_crc = modbus_crc16(response, frame_len - 2);
    if ((calc_crc & 0xFF) != response[frame_len - 2] ||
        ((calc_crc >> 8) & 0xFF) != response[frame_len - 1]) 
    {
            printf("BMS Bad CRC -> 0x%X, Expected: 0x%X;\n", (response[frame_len - 2] | (response[frame_len - 1] << 8)), calc_crc);
            return MODBUS_ERR_CRC;
    }

    ptr = &response[3];
    for (i = 0; i < reg_count; i++) 
    {
        dest_buffer[i] = *(ptr+2*i) | (*(ptr+2*i+1) << 8);
    }

    return MODBUS_OK;
}

/**
 * @brief 写单个寄存器（功能码 0x06）
 * @param slave_addr     从机地址（1~247）
 * @param reg_addr       寄存器地址（16 位）
 * @param value          写入的 16 位数据
 * @return               0 成功，其他为错误码
 */
uint8_t Modbus_WriteSingleRegister(uint8_t slave_addr, uint16_t reg_addr, uint16_t value)
{
    uint8_t request[8];      /* 请求帧固定 8 字节 */
    uint8_t response[8];     /* 响应帧固定 8 字节（回显） */
    uint16_t crc;
    uint16_t recv_len;
    uint16_t calc_crc;

    /* 组帧（注意：Modbus 寄存器数据为大端） */
    request[0] = slave_addr;                     /* 从机地址 */
    request[1] = 0x06;                           /* 功能码：写单个寄存器 */
    request[2] = (uint8_t)(reg_addr >> 8);       /* 寄存器地址高字节 */
    request[3] = (uint8_t)(reg_addr & 0xFF);     /* 寄存器地址低字节 */
    request[4] = (uint8_t)(value & 0xFF);        /* 数据低字节 */
    request[5] = (uint8_t)(value >> 8);          /* 数据高字节 */
    crc = modbus_crc16(request, 6);              /* 计算 CRC（前 6 字节） */
    request[6] = (uint8_t)(crc & 0xFF);          /* CRC 低字节 */
    request[7] = (uint8_t)(crc >> 8);            /* CRC 高字节 */

    if (bms_send(request, 8) != 8) {
        return MODBUS_ERR_TX_FAIL;
    }

    recv_len = 8;
    if (bms_receive(response, recv_len, RX_TIMEOUT_MS) != RT_EOK) {
        return MODBUS_ERR_TIMEOUT;
    }

    /* 等待从机回显 */
    if (response[0] != slave_addr) {
        return MODBUS_ERR_SLAVE_ADDR;
    }
    if (response[1] != 0x06) {
        return MODBUS_ERR_FUNC_CODE;
    }

    /* 校验 CRC */
    calc_crc = modbus_crc16(response, 6);   /* 前 6 字节的 CRC */
    if ((calc_crc & 0xFF) != response[6] ||
        ((calc_crc >> 8) & 0xFF) != response[7]) {
        return MODBUS_ERR_CRC;
    }

    /* 回显校验：地址、功能码和数据必须与请求一致 */
    if (response[2] != request[2] || response[3] != request[3] ||
        response[4] != request[4] || response[5] != request[5]) {
        /* 如果回显不一致，返回长度错误（部分设备可能不回显） */
        /* 当前按严格模式处理 */
        return MODBUS_ERR_FRAME_LEN;
    }

    return MODBUS_OK;
}

static rt_device_t bms_dev;

static int bms_send(const uint8_t *data, uint16_t length)
{
    if (bms_dev == RT_NULL)
    {
        return 0;
    }
    return (int)rt_device_write(bms_dev, 0, data, length);
}

int bms_init(void)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

    bms_dev = rt_device_find("uart1");
    if (bms_dev == RT_NULL)
    {
        return -1;
    }

    cfg.baud_rate = BAUD_RATE_115200;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    cfg.parity = PARITY_NONE;

    rt_device_control(bms_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
    rt_device_open(bms_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    return 0;
}

static rt_err_t bms_receive(uint8_t *buffer, uint16_t length, uint32_t timeout_ms)
{
    uint16_t received = 0;
    uint32_t last_tick = bsp_tick_get();

    while (received < length)
    {
        int count = (int)rt_device_read(bms_dev, 0, buffer + received, length - received);

        if (count > 0)
        {
            received += (uint16_t)count;
            last_tick = bsp_tick_get();
        }
        else
        {
            if ((bsp_tick_get() - last_tick) >= timeout_ms)
            {
                return -RT_ETIMEOUT;
            }
            rt_thread_mdelay(1);
        }
    }

    return RT_EOK;
}

int bms_read_soc(uint16_t * soc)
{
    return (Modbus_ReadHoldingRegisters(0x01, 0x9028, 1, soc, 2) == MODBUS_OK) ? 0 : 1;
}
//        bsp_delay_ms(50);
int bms_read_soh(uint16_t * soh)
{
    return (Modbus_ReadHoldingRegisters(0x01, 0x9029, 1, soh, 2) == MODBUS_OK) ? 0 : 1;
}

/* Modbus CRC16 查表法（多项式 0xA001） */
uint16_t modbus_crc16(const uint8_t *pucFrame, uint16_t usLen)
{
    static const uint8_t aucCRCHi[] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40
    };
    static const uint8_t aucCRCLo[] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
        0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
        0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
        0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
        0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
        0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
        0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
        0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
        0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
        0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
        0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
        0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
        0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
        0x41, 0x81, 0x80, 0x40
    };

    uint8_t ucCRCHi = 0xFF;
    uint8_t ucCRCLo = 0xFF;
    int iIndex;
    uint16_t crclen = usLen;

    while (crclen--) {
        iIndex = ucCRCLo ^ *(pucFrame++);
        ucCRCLo = (uint8_t)(ucCRCHi ^ aucCRCHi[iIndex]);
        ucCRCHi = aucCRCLo[iIndex];
    }
    return (uint16_t)(ucCRCHi << 8 | ucCRCLo);
}
