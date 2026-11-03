

// SPDX-License-Identifier: GPL-3.0
// Copyright (C) 2026 zz020


#include <stddef.h>
#include "bsp.h"
#include "pinout.h"

#include "ch224.h"
#include "i2c_soft.h"

uint8_t CH224_ReadStatus(uint8_t *voltage_status, CH224_I2C_Status *i2c_status)
{
    uint8_t data;
    if (I2C_ReadReg(CH224_ADDR, 0x09, &data)) 
		return 1;
    if (i2c_status)
	{
		i2c_status->BC  = data & 0x01;
		i2c_status->QC2 = (data>>1) & 0x01;
		i2c_status->QC3 = (data>>2) & 0x01;
		i2c_status->PD  = (data>>3) & 0x01;
		i2c_status->EPR = (data>>4) & 0x01;
		i2c_status->Rev = (data>>5) & 0x07;
	}

	if (I2C_ReadReg(CH224_ADDR, 0x50, &i2c_status->Max_I))
		return 3;
    return 0;
}

uint8_t CH224_ReadCurrent(void)
{
    uint8_t cur;
    if (I2C_ReadReg(CH224_ADDR, 0x50, &cur)) 
		return 0;
    return cur;
}

uint8_t CH224_ReadAVS(uint16_t *avs_value)
{
    uint8_t avs_h, avs_l;
    if (I2C_ReadReg(CH224_ADDR, 0x51, &avs_h)) 
		return 1;
    if (I2C_ReadReg(CH224_ADDR, 0x52, &avs_l)) 
		return 2;
    *avs_value = (avs_h << 8) | avs_l;
    return 0;
}

uint8_t CH224_ReadPPS(void)
{
    uint8_t pps;
    if (I2C_ReadReg(CH224_ADDR, 0x53, &pps)) 
		return 0;
    return pps;
}

void CH224_RequestFixed(uint8_t voltage_code)
{
    I2C_WriteReg(CH224_ADDR, 0x0A, voltage_code);
}

void CH224_RequestPPS(uint8_t voltage_0_1V)
{
    I2C_WriteReg(CH224_ADDR, 0x53, voltage_0_1V);
    I2C_WriteReg(CH224_ADDR, 0x0A, 6);  // 请求 PPS
}

void CH224_RequestAVS(uint16_t voltage_0_1V)
{
    uint8_t dataL = voltage_0_1V & 0xFF;
    uint8_t dataH = (voltage_0_1V >> 8) | 0x80;
    I2C_WriteReg(CH224_ADDR, 0x52, dataL);
    I2C_WriteReg(CH224_ADDR, 0x51, dataH);
    I2C_WriteReg(CH224_ADDR, 0x0A, 7);  // 请求 AVS
}

void CH224_ReadAdaptorInfo(uint8_t * adaptInfo)
{
	if(adaptInfo != NULL)
		I2C_ReadData(CH224_ADDR, adaptInfo, 0x30);
}

uint8_t CH224_ReRequestPPS(void)
{
    uint8_t status = 0;

    status = I2C_WriteReg(CH224_ADDR, 0x0A, 0x05);
    if (status) return 1;

    return 0;
}