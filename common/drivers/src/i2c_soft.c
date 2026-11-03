

// SPDX-License-Identifier: GPL-3.0
// Copyright (C) 2026 zz020


#include "bsp.h"
#include "pinout.h"

#include "i2c_soft.h"


void I2C_SOFT_INIT()
{
    bsp_pin_mode(BOARD_PIN_I2C_SCL, BSP_PIN_OUTPUT_OD);
    bsp_pin_mode(BOARD_PIN_I2C_SDA, BSP_PIN_OUTPUT_OD);
#ifndef PMIC_LEFT_LAYOUT
    bsp_pin_mode(BOARD_PIN_OLED_SCL, BSP_PIN_OUTPUT_OD);
    bsp_pin_mode(BOARD_PIN_OLED_SDA, BSP_PIN_OUTPUT_OD);
#endif

    bsp_pin_write(BOARD_PIN_I2C_SCL, 1);
    bsp_pin_write(BOARD_PIN_I2C_SDA, 1);
#ifndef PMIC_LEFT_LAYOUT
    bsp_pin_write(BOARD_PIN_OLED_SCL, 1);
    bsp_pin_write(BOARD_PIN_OLED_SDA, 1);
#endif
}

// 微秒级延时（按 72MHz 粗略校准，具体以实测为准）
void I2C_Delay_us(uint32_t us)
{
    bsp_delay_us(us);
}

// I2C 起始条件
void I2C_Start(void)
{
    I2C_SDA_HIGH();
    I2C_SCL_HIGH();
    I2C_Delay_us(5);
    I2C_SDA_LOW();
    I2C_Delay_us(5);
    I2C_SCL_LOW();
}

// I2C 停止条件
void I2C_Stop(void)
{
    I2C_SDA_LOW();
    I2C_SCL_HIGH();
    I2C_Delay_us(5);
    I2C_SDA_HIGH();
    I2C_Delay_us(5);
}

// 发送一个字节（MSB 先发）
void I2C_SendByte(uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            I2C_SDA_HIGH();
        else
            I2C_SDA_LOW();
        data <<= 1;
        I2C_Delay_us(2);
        I2C_SCL_HIGH();
        I2C_Delay_us(5);
        I2C_SCL_LOW();
        I2C_Delay_us(2);
    }
}

// 读取一个字节
uint8_t I2C_ReadByte(void)
{
    uint8_t i, data = 0;
    I2C_SDA_HIGH();  // 释放 SDA，准备接收
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        I2C_SCL_HIGH();
        I2C_Delay_us(2);
        if (I2C_SDA_READ())
            data |= 0x01;
        I2C_SCL_LOW();
        I2C_Delay_us(2);
    }
    return data;
}

// 等待从机应答，返回 0:ACK，1:NACK
uint8_t I2C_WaitAck(void)
{
    uint8_t ack;
    I2C_SDA_HIGH();  // 释放 SDA，等待应答
    I2C_Delay_us(2);
    I2C_SCL_HIGH();
    I2C_Delay_us(2);
    ack = I2C_SDA_READ();  // 0:ACK, 1:NACK
    I2C_SCL_LOW();
    I2C_Delay_us(2);
    return ack;
}

// 发送应答 ACK
void I2C_SendAck(void)
{
    I2C_SDA_LOW();
    I2C_Delay_us(2);
    I2C_SCL_HIGH();
    I2C_Delay_us(5);
    I2C_SCL_LOW();
    I2C_SDA_HIGH();
}

// 发送非应答 NACK
void I2C_SendNoAck(void)
{
    I2C_SDA_HIGH();
    I2C_Delay_us(2);
    I2C_SCL_HIGH();
    I2C_Delay_us(5);
    I2C_SCL_LOW();
    I2C_SDA_LOW();
}

// 向寄存器写一个字节（TMP100、CH224 等）
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    I2C_Start();
    I2C_SendByte(dev_addr << 1);  // 发送设备地址（写方向）
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_SendByte(reg_addr);
    if (I2C_WaitAck()) { I2C_Stop(); return 2; }
    I2C_SendByte(data);
    if (I2C_WaitAck()) { I2C_Stop(); return 3; }
    I2C_Stop();
    return 0;
}

// 从寄存器读一个字节
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    I2C_Start();
    I2C_SendByte(dev_addr << 1);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_SendByte(reg_addr);
    if (I2C_WaitAck()) { I2C_Stop(); return 2; }
    I2C_Start();
    I2C_SendByte((dev_addr << 1) | 0x01);
    if (I2C_WaitAck()) { I2C_Stop(); return 3; }
    *data = I2C_ReadByte();
    I2C_SendNoAck();
    I2C_Stop();
    return 0;
}

// 连续写数据（不带寄存器地址）
uint8_t I2C_WriteData(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    I2C_Start();
    I2C_SendByte(dev_addr << 1);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    for (uint16_t i = 0; i < len; i++)
    {
        I2C_SendByte(data[i]);
        if (I2C_WaitAck()) { I2C_Stop(); return 2; }
    }
    I2C_Stop();
    return 0;
}

// 连续读数据（不带寄存器地址）
uint8_t I2C_ReadData(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    I2C_Start();
    I2C_SendByte((dev_addr << 1) | 0x01);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = I2C_ReadByte();
        if (i == len - 1)
            I2C_SendNoAck();
        else
            I2C_SendAck();
    }
    I2C_Stop();
    return 0;
}

// 向指定寄存器连续写数据
uint8_t I2C_WriteDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    I2C_Start();
    I2C_SendByte(dev_addr << 1);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_SendByte(reg_addr);
    if (I2C_WaitAck()) { I2C_Stop(); return 2; }
    for (uint16_t i = 0; i < len; i++)
    {
        I2C_SendByte(data[i]);
        if (I2C_WaitAck()) { I2C_Stop(); return 3; }
    }
    I2C_Stop();
    return 0;
}

// 从指定寄存器连续读数据
uint8_t I2C_ReadDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    I2C_Start();
    I2C_SendByte(dev_addr << 1);
    if (I2C_WaitAck()) { I2C_Stop(); return 1; }
    I2C_SendByte(reg_addr);
    if (I2C_WaitAck()) { I2C_Stop(); return 2; }
    I2C_Start();
    I2C_SendByte((dev_addr << 1) | 0x01);
    if (I2C_WaitAck()) { I2C_Stop(); return 3; }
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = I2C_ReadByte();
        if (i == len - 1)
            I2C_SendNoAck();
        else
            I2C_SendAck();
    }
    I2C_Stop();
    return 0;
}


#ifndef PMIC_LEFT_LAYOUT
void OLED_I2C_Delay_us(uint32_t us)
{
    bsp_delay_us(us);
}


void OLED_I2C_Start(void)
{
    OLED_I2C_SDA_HIGH();
    OLED_I2C_SCL_HIGH();
    OLED_I2C_Delay_us(5);
    OLED_I2C_SDA_LOW();
    OLED_I2C_Delay_us(5);
    OLED_I2C_SCL_LOW();
}

// I2C 停止条件
void OLED_I2C_Stop(void)
{
    OLED_I2C_SDA_LOW();
    OLED_I2C_SCL_HIGH();
    OLED_I2C_Delay_us(5);
    OLED_I2C_SDA_HIGH();
    OLED_I2C_Delay_us(5);
}

// 发送一个字节（MSB 先发）
void OLED_I2C_SendByte(uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            OLED_I2C_SDA_HIGH();
        else
            OLED_I2C_SDA_LOW();
        data <<= 1;
        OLED_I2C_Delay_us(2);
        OLED_I2C_SCL_HIGH();
        OLED_I2C_Delay_us(5);
        OLED_I2C_SCL_LOW();
        OLED_I2C_Delay_us(2);
    }
}

// 读取一个字节
uint8_t OLED_I2C_ReadByte(void)
{
    uint8_t i, data = 0;
    OLED_I2C_SDA_HIGH();  // 释放 SDA，准备接收
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        OLED_I2C_SCL_HIGH();
        OLED_I2C_Delay_us(2);
        if (OLED_I2C_SDA_READ())
            data |= 0x01;
        OLED_I2C_SCL_LOW();
        OLED_I2C_Delay_us(2);
    }
    return data;
}

// 等待从机应答，返回 0:ACK，1:NACK
uint8_t OLED_I2C_WaitAck(void)
{
    uint8_t ack;
    OLED_I2C_SDA_HIGH();  // 释放 SDA，等待应答
    OLED_I2C_Delay_us(2);
    OLED_I2C_SCL_HIGH();
    OLED_I2C_Delay_us(2);
    ack = OLED_I2C_SDA_READ();  // 0:ACK, 1:NACK
    OLED_I2C_SCL_LOW();
    OLED_I2C_Delay_us(2);
    return ack;
}

// 发送应答 ACK
void OLED_I2C_SendAck(void)
{
    OLED_I2C_SDA_LOW();
    OLED_I2C_Delay_us(2);
    OLED_I2C_SCL_HIGH();
    OLED_I2C_Delay_us(5);
    OLED_I2C_SCL_LOW();
    OLED_I2C_SDA_HIGH();
}

// 发送非应答 NACK
void OLED_I2C_SendNoAck(void)
{
    OLED_I2C_SDA_HIGH();
    OLED_I2C_Delay_us(2);
    OLED_I2C_SCL_HIGH();
    OLED_I2C_Delay_us(5);
    OLED_I2C_SCL_LOW();
    OLED_I2C_SDA_LOW();
}

// 向寄存器写一个字节（TMP100、CH224 等）
uint8_t OLED_I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(dev_addr << 1);  // 发送设备地址（写方向）
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 1; }
    OLED_I2C_SendByte(reg_addr);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 2; }
    OLED_I2C_SendByte(data);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 3; }
    OLED_I2C_Stop();
    return 0;
}

// 从寄存器读一个字节
uint8_t OLED_I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(dev_addr << 1);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 1; }
    OLED_I2C_SendByte(reg_addr);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 2; }
    OLED_I2C_Start();
    OLED_I2C_SendByte((dev_addr << 1) | 0x01);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 3; }
    *data = OLED_I2C_ReadByte();
    OLED_I2C_SendNoAck();
    OLED_I2C_Stop();
    return 0;
}

// 连续写数据（不带寄存器地址）
uint8_t OLED_I2C_WriteData(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(dev_addr << 1);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 1; }
    for (uint16_t i = 0; i < len; i++)
    {
        OLED_I2C_SendByte(data[i]);
        if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 2; }
    }
    OLED_I2C_Stop();
    return 0;
}

// 连续读数据（不带寄存器地址）
uint8_t OLED_I2C_ReadData(uint8_t dev_addr, uint8_t *data, uint16_t len)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte((dev_addr << 1) | 0x01);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 1; }
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = OLED_I2C_ReadByte();
        if (i == len - 1)
            OLED_I2C_SendNoAck();
        else
            OLED_I2C_SendAck();
    }
    OLED_I2C_Stop();
    return 0;
}

// 向指定寄存器连续写数据
uint8_t OLED_I2C_WriteDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(dev_addr << 1);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 1; }
    OLED_I2C_SendByte(reg_addr);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 2; }
    for (uint16_t i = 0; i < len; i++)
    {
        OLED_I2C_SendByte(data[i]);
        if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 3; }
    }
    OLED_I2C_Stop();
    return 0;
}

// 从指定寄存器连续读数据
uint8_t OLED_I2C_ReadDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(dev_addr << 1);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 1; }
    OLED_I2C_SendByte(reg_addr);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 2; }
    OLED_I2C_Start();
    OLED_I2C_SendByte((dev_addr << 1) | 0x01);
    if (OLED_I2C_WaitAck()) { OLED_I2C_Stop(); return 3; }
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = OLED_I2C_ReadByte();
        if (i == len - 1)
            OLED_I2C_SendNoAck();
        else
            OLED_I2C_SendAck();
    }
    OLED_I2C_Stop();
    return 0;
}

#endif /* PMIC_LEFT_LAYOUT */

