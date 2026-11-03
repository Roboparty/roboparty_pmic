

#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

#define OLED_ADDR        0x3C   // 7 位地址，写地址为 0x78
#define OLED_CMD         0x00
#define OLED_DATA        0x40

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t decimal, uint8_t size);
void OLED_SetDirection(uint8_t flag);

void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1, const unsigned char BMP[]);

extern const unsigned char gImage_PD_PWR[1024];
extern const unsigned char gImage_SUM_INFO[1024];
extern const unsigned char gImage_12x12[][16];
extern void OLED_WriteCmd(uint8_t cmd);
extern const unsigned char gImage_BBWW[][16];

#endif

