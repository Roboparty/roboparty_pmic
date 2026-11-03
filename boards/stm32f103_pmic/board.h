#ifndef __BOARD_H__
#define __BOARD_H__

#include "stm32f103xb.h"
#include <rtthread.h>

void SystemClock_Config(void);
void rt_hw_serial_init(void);
void rt_hw_pin_init(void);

#endif /* __BOARD_H__ */
