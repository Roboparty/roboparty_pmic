

#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#include "bsp.h"
#include "pinout.h"

// 逻辑引脚映射由 boards/<board>/pinout.h 提供

#define OLED_I2C_SCL_HIGH()     bsp_pin_write(BOARD_PIN_OLED_SCL, 1)
#define OLED_I2C_SCL_LOW()      bsp_pin_write(BOARD_PIN_OLED_SCL, 0)
#define OLED_I2C_SDA_HIGH()     bsp_pin_write(BOARD_PIN_OLED_SDA, 1)
#define OLED_I2C_SDA_LOW()      bsp_pin_write(BOARD_PIN_OLED_SDA, 0)
#define OLED_I2C_SDA_READ()     bsp_pin_read(BOARD_PIN_OLED_SDA)

#define I2C_SCL_HIGH()          bsp_pin_write(BOARD_PIN_I2C_SCL, 1)
#define I2C_SCL_LOW()           bsp_pin_write(BOARD_PIN_I2C_SCL, 0)
#define I2C_SDA_HIGH()          bsp_pin_write(BOARD_PIN_I2C_SDA, 1)
#define I2C_SDA_LOW()           bsp_pin_write(BOARD_PIN_I2C_SDA, 0)
#define I2C_SDA_READ()          bsp_pin_read(BOARD_PIN_I2C_SDA)

void I2C_SOFT_INIT();

void I2C_Delay_us(uint32_t us);

// I2C 基本时序
void I2C_Start(void);
void I2C_Stop(void);
void I2C_SendByte(uint8_t data);
uint8_t I2C_ReadByte(void);
uint8_t I2C_WaitAck(void);
void I2C_SendAck(void);
void I2C_SendNoAck(void);

// 寄存器读写
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);
uint8_t I2C_WriteData(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t I2C_ReadData(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t I2C_WriteDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
uint8_t I2C_ReadDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);


void OLED_I2C_SOFT_INIT();

void OLED_I2C_Delay_us(uint32_t us);

// I2C 基本时序
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_SendByte(uint8_t data);
uint8_t OLED_I2C_ReadByte(void);
uint8_t OLED_I2C_WaitAck(void);
void OLED_I2C_SendAck(void);
void OLED_I2C_SendNoAck(void);

// 寄存器读写
uint8_t OLED_I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
uint8_t OLED_I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);
uint8_t OLED_I2C_WriteData(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t OLED_I2C_ReadData(uint8_t dev_addr, uint8_t *data, uint16_t len);
uint8_t OLED_I2C_WriteDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
uint8_t OLED_I2C_ReadDataWithReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

#endif

