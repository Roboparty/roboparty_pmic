

#ifndef __CH224_H
#define __CH224_H

#include <stdint.h>

// CH224 7 位地址
#define CH224_ADDR         0x23

// 芯片状态
typedef struct {
    uint8_t BC:1;
    uint8_t QC2:1;
    uint8_t QC3:1;
    uint8_t PD:1;
    uint8_t EPR:1;
    uint8_t Rev:3;

		uint8_t Max_I;
} CH224_I2C_Status;

uint8_t CH224_ReadStatus(uint8_t *voltage_status, CH224_I2C_Status *i2c_status);
uint8_t CH224_ReadCurrent(void);
uint8_t CH224_ReadAVS(uint16_t *avs_value);
uint8_t CH224_ReadPPS(void);
void CH224_RequestFixed(uint8_t voltage_code);  // 0:5V,1:9V,2:12V,3:15V,4:20V,5:28V
void CH224_RequestPPS(uint8_t voltage_0_1V);    // 电压单位 0.1V，例如 50 表示 5.0V
void CH224_RequestAVS(uint16_t voltage_0_1V);   // 电压单位 0.01V，例如 500 表示 5.00V
void CH224_ReadAdaptorInfo(uint8_t * adaptInfo);
uint8_t CH224_ReRequestPPS(void);

#endif

