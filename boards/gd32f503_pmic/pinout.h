#ifndef __BOARD_PINOUT_H__
#define __BOARD_PINOUT_H__

#include "bsp.h"

/* Placeholder mapping; replace with the RP1.4 schematic pins. */
#define BOARD_PIN_BUZZER        BSP_PIN(0, 4)
#define BOARD_PIN_KEY1          BSP_PIN(0, 5)
#define BOARD_PIN_CHARGE_EN     BSP_PIN(1, 3)
#define BOARD_PIN_BAT_INST      BSP_PIN(1, 4)
#define BOARD_PIN_IR_REC        BSP_PIN(2, 0)
#define BOARD_PIN_IR_EM         BSP_PIN(2, 1)
#define BOARD_PIN_OLED_SCL      BSP_PIN(2, 2)
#define BOARD_PIN_OLED_SDA      BSP_PIN(2, 3)
#define BOARD_PIN_I2C_SCL       BSP_PIN(1, 8)
#define BOARD_PIN_I2C_SDA       BSP_PIN(1, 9)
#define BOARD_PIN_SPI_CS        BSP_PIN(1, 12)

#define OTA_MAX_SIZE            (224U * 1024U)
#define OTA_APP_ENTRY           0x08008000U

#endif /* __BOARD_PINOUT_H__ */
