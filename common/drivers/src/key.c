

// SPDX-License-Identifier: GPL-3.0
// Copyright (C) 2026 zz020




#include "key.h"

typedef enum {
    KEY_IDLE,               // 空闲
    KEY_PRESS_WAIT,         // 按下消抖
    KEY_PRESSED,            // 已按下（等待松开或长按）
    KEY_RELEASE_WAIT,       // 松开消抖
    KEY_WAIT_DOUBLE,        // 等待双击
    KEY_DOUBLE_DONE,        // 双击完成，等待释放
    KEY_LONG_PRESSED        // 长按已触发，等待释放
} KeyState;

static KeyState keyState = KEY_IDLE;
static uint32_t lastTick = 0;
static uint32_t pressTick = 0;   // 按下时刻

// 时间参数（单位：ms）
#define DEBOUNCE_TIME      20
#define DOUBLE_CLICK_TIME  300
#define LONG_PRESS_TIME    1000    // 长按判定时间

// 按键扫描
uint8_t KEY1_EVENT = 0;
uint8_t KEY2_EVENT = 0;
uint8_t KEY_LONG_EVENT = 0;
uint8_t KEY1_VAL = 1;
uint8_t KEY2_VAL = 1;

void KEY_Scan(void)
{
    uint32_t now = bsp_tick_get();
    uint8_t level = bsp_pin_read(BOARD_PIN_KEY1);
    uint8_t isPressed = (level == 0);

    switch (keyState)
    {
        case KEY_IDLE:
            if (isPressed) {
                keyState = KEY_PRESS_WAIT;
                lastTick = now;
            }
            break;

        case KEY_PRESS_WAIT:
            if (isPressed) {
                if ((now - lastTick) >= DEBOUNCE_TIME) {
                    pressTick = now;          // 记录按下时刻
                    keyState = KEY_PRESSED;
                }
            } else {
                keyState = KEY_IDLE;          // 抖动，重新等待
            }
            break;

        case KEY_PRESSED:
            if (!isPressed) {
                // 松开，进入松开消抖（区分单击/双击）
                keyState = KEY_RELEASE_WAIT;
                lastTick = now;
            } else {
                // 保持按下
                if ((now - pressTick) >= LONG_PRESS_TIME) {
                    // 达到长按时间
                    KEY_LONG_EVENT = 1;
                    keyState = KEY_LONG_PRESSED;   // 等待松开，避免重复触发
                    lastTick = now;
                }
            }
            break;

        case KEY_RELEASE_WAIT:
            if (!isPressed) {
                if ((now - lastTick) >= DEBOUNCE_TIME) {
                    // 确认松开，等待双击
                    keyState = KEY_WAIT_DOUBLE;
                    lastTick = now;
                }
            } else {
                // 再次按下，取消单击
                keyState = KEY_PRESS_WAIT;
                lastTick = now;
            }
            break;

        case KEY_WAIT_DOUBLE:
            if (isPressed) {
                if ((now - lastTick) >= DEBOUNCE_TIME) {
                    // 双击间隔内再次按下
                    KEY2_EVENT = 1;
                    keyState = KEY_DOUBLE_DONE;
                    lastTick = now;
                }
            } else {
                if ((now - lastTick) >= DOUBLE_CLICK_TIME) {
                    // 超过双击间隔，判定为单击
                    KEY1_EVENT = 1;
                    keyState = KEY_IDLE;
                }
            }
            break;

        case KEY_DOUBLE_DONE:
            // 双击完成，等待释放
            if (!isPressed) {
                if ((now - lastTick) >= DEBOUNCE_TIME) {
                    keyState = KEY_IDLE;
                }
            } else {
                lastTick = now;
            }
            break;

        case KEY_LONG_PRESSED:
            // 长按完成，等待释放
            if (!isPressed) {
                if ((now - lastTick) >= DEBOUNCE_TIME) {
                    keyState = KEY_IDLE;
                }
            } else {
                lastTick = now;
            }
            break;
    }
}
