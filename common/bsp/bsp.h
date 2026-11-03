#ifndef __BSP_H__
#define __BSP_H__

#include <stdint.h>

typedef uint16_t bsp_pin_t;

#define BSP_PIN(port, pin)      (((uint16_t)(port) << 4) | ((uint16_t)(pin) & 0x0F))
#define BSP_PIN_PORT(pin)       ((uint8_t)((pin) >> 4))
#define BSP_PIN_INDEX(pin)      ((uint8_t)((pin) & 0x0F))

enum
{
    BSP_PIN_OUTPUT = 0,
    BSP_PIN_OUTPUT_OD,
    BSP_PIN_INPUT,
    BSP_PIN_INPUT_PULLUP,
    BSP_PIN_INPUT_PULLDOWN,
    BSP_PIN_ANALOG,
    BSP_PIN_AF_PP,
};

enum
{
    BSP_PWM_FAN = 0,
    BSP_PWM_CHARGE,
    BSP_PWM_LED_R,
    BSP_PWM_LED_G,
    BSP_PWM_LED_B,
};

void bsp_pin_mode(bsp_pin_t pin, uint8_t mode);
void bsp_pin_write(bsp_pin_t pin, int value);
int bsp_pin_read(bsp_pin_t pin);
void bsp_delay_us(uint32_t us);

void bsp_pwm_init(void);
void bsp_pwm_set(int channel, uint16_t value);

extern volatile uint16_t bsp_adc_values[3];
void bsp_adc_init(void);
int bsp_adc_start(void);
void bsp_adc_stop(void);
void bsp_adc_complete(void);

int bsp_flash_erase_page(uint32_t address);
int bsp_flash_write_halfword(uint32_t address, uint16_t data);

uint32_t bsp_tick_get(void);
void bsp_delay_ms(uint32_t ms);

void Error_Handler(void);

#endif /* __BSP_H__ */
