#ifndef __KEY_H
#define __KEY_H

#include "bsp.h"
#include "pinout.h"

// 按键事件
extern uint8_t KEY1_EVENT;   // 单击事件
extern uint8_t KEY2_EVENT;   // 双击事件
extern uint8_t KEY_LONG_EVENT; // 长按事件

// 按键状态（按下为 1，松开为 0）
extern uint8_t KEY1_VAL;
extern uint8_t KEY2_VAL;

void KEY_Init(void);
void KEY_Scan(void);

#endif