
// SPDX-License-Identifier: GPL-3.0
// Copyright (C) 2026 zz020


#include "bsp.h"
#include "pinout.h"

#include "tmp100.h"
#include "i2c_soft.h"

float TMP100_GetTemperature(void)
{
    uint8_t temp_reg[2];
    // 读取温度寄存器（TMP100 温度寄存器地址 0x00，共 2 字节）
    if (I2C_ReadDataWithReg(TMP100_ADDR, 0x00, temp_reg, 2))
        return -273.15f;  // 读取失败时返回绝对零度
    int16_t raw = (temp_reg[0] << 8) | temp_reg[1];
	
    raw >>= 4;  // TMP100 为 12 位温度值，右移 4 位
	//printf("temp raw = 0x%X", raw);
	
    return raw * 0.0625f;
}
