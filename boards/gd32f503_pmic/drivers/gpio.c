#include "bsp.h"
#include "gd32f50x_gpio.h"

static uint32_t port_of(bsp_pin_t pin)
{
    return BSP_PIN_PORT(pin) == 0U ? GPIOA : GPIOB;
}

void bsp_pin_mode(bsp_pin_t pin, uint8_t mode)
{
    uint32_t gpio = port_of(pin);
    uint32_t bit = 1UL << BSP_PIN_INDEX(pin);
    uint32_t pull = GPIO_PUPD_NONE;
    uint32_t gd_mode = GPIO_MODE_INPUT;

    if (mode == BSP_PIN_OUTPUT || mode == BSP_PIN_OUTPUT_OD)
    {
        gd_mode = GPIO_MODE_OUTPUT;
    }
    else if (mode == BSP_PIN_ANALOG)
    {
        gd_mode = GPIO_MODE_ANALOG;
    }
    else if (mode == BSP_PIN_AF_PP)
    {
        gd_mode = GPIO_MODE_AF;
    }
    else if (mode == BSP_PIN_INPUT_PULLUP)
    {
        pull = GPIO_PUPD_PULLUP;
    }
    else if (mode == BSP_PIN_INPUT_PULLDOWN)
    {
        pull = GPIO_PUPD_PULLDOWN;
    }
    gpio_mode_set(gpio, gd_mode, pull, bit);
    if (gd_mode == GPIO_MODE_OUTPUT || gd_mode == GPIO_MODE_AF)
    {
        gpio_output_options_set(gpio, mode == BSP_PIN_OUTPUT_OD ? GPIO_OTYPE_OD : GPIO_OTYPE_PP,
                                GPIO_OSPEED_LEVEL1, bit);
    }
}

void bsp_pin_write(bsp_pin_t pin, int value)
{
    uint32_t gpio = port_of(pin);
    uint32_t bit = 1UL << BSP_PIN_INDEX(pin);
    if (value) gpio_bit_set(gpio, bit);
    else gpio_bit_reset(gpio, bit);
}

int bsp_pin_read(bsp_pin_t pin)
{
    return gpio_input_bit_get(port_of(pin), 1UL << BSP_PIN_INDEX(pin)) == SET;
}

void bsp_delay_us(uint32_t us)
{
    uint32_t loops = SystemCoreClock / 5000000U;
    for (uint32_t i = 0; i < us; ++i)
    {
        for (volatile uint32_t j = 0; j < loops; ++j) __NOP();
    }
}
