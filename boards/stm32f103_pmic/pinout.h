#ifndef __BOARD_PINOUT_H__
#define __BOARD_PINOUT_H__

#include "bsp.h"

#ifdef PMIC_LEFT_LAYOUT
#define BOARD_PIN_LED_R         BSP_PIN(0, 4)
#define BOARD_PIN_LED_G         BSP_PIN(0, 5)
#define BOARD_PIN_BAT_G         BSP_PIN(0, 6)
#define BOARD_PIN_BAT_R         BSP_PIN(0, 7)
#else
#define BOARD_PIN_BUZZER        BSP_PIN(0, 4)
#define BOARD_PIN_KEY1          BSP_PIN(0, 5)
#endif
#define BOARD_PIN_CHARGE_EN     BSP_PIN(1, 3)
#ifndef PMIC_LEFT_LAYOUT
#define BOARD_PIN_BAT_INST      BSP_PIN(1, 4)
#define BOARD_PIN_IR_REC        BSP_PIN(1, 6)
#define BOARD_PIN_IR_EM         BSP_PIN(1, 7)
#define BOARD_PIN_OLED_SCL      BSP_PIN(1, 1)
#define BOARD_PIN_OLED_SDA      BSP_PIN(1, 2)
#define BOARD_PIN_SPI_CS        BSP_PIN(1, 12)
#endif
#define BOARD_PIN_I2C_SCL       BSP_PIN(1, 8)
#define BOARD_PIN_I2C_SDA       BSP_PIN(1, 9)

#define OTA_MAX_SIZE            (59U * 1024U)
#define OTA_APP_ENTRY           0x08001000U

#endif /* __BOARD_PINOUT_H__ */
