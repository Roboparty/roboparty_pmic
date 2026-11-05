#ifndef __BOARD_PINOUT_H__
#define __BOARD_PINOUT_H__

#include "bsp.h"

/* Reference GD32F503VET6 layout. Hardware must match before flashing. */
#define BOARD_PIN_LED_R         BSP_PIN(0, 4)
#define BOARD_PIN_LED_G         BSP_PIN(0, 5)
#define BOARD_PIN_BAT_G         BSP_PIN(0, 6)
#define BOARD_PIN_BAT_R         BSP_PIN(0, 7)
#define BOARD_PIN_CHARGE_EN     BSP_PIN(1, 3)
#define BOARD_PIN_I2C_SCL       BSP_PIN(1, 8)
#define BOARD_PIN_I2C_SDA       BSP_PIN(1, 9)

#define OTA_MAX_SIZE            ((224U * 1024U) - 0x100U)
#define OTA_APP_ENTRY           0x08008000U

#endif /* __BOARD_PINOUT_H__ */
