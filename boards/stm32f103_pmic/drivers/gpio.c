#include "board.h"
#include "bsp.h"

static GPIO_TypeDef *pin_port(bsp_pin_t pin)
{
    return (BSP_PIN_PORT(pin) == 0U) ? GPIOA : GPIOB;
}

void bsp_pin_mode(bsp_pin_t pin, uint8_t mode)
{
    GPIO_TypeDef *gpio = pin_port(pin);
    uint32_t index = BSP_PIN_INDEX(pin);
    volatile uint32_t *reg = (index < 8U) ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (index & 0x07U) * 4U;
    uint32_t value;

    switch (mode)
    {
    case BSP_PIN_OUTPUT:
        value = 0x2U;
        break;
    case BSP_PIN_OUTPUT_OD:
        value = 0x6U;
        break;
    case BSP_PIN_INPUT:
        value = 0x4U;
        break;
    case BSP_PIN_INPUT_PULLUP:
        value = 0x8U;
        gpio->BSRR = 1UL << index;
        break;
    case BSP_PIN_INPUT_PULLDOWN:
        value = 0x8U;
        gpio->BRR = 1UL << index;
        break;
    case BSP_PIN_ANALOG:
        value = 0x0U;
        break;
    case BSP_PIN_AF_PP:
        value = 0xBU;
        break;
    default:
        value = 0x4U;
        break;
    }

    *reg = (*reg & ~(0x0FU << shift)) | (value << shift);
}

void bsp_pin_write(bsp_pin_t pin, int value)
{
    GPIO_TypeDef *gpio = pin_port(pin);
    uint32_t index = BSP_PIN_INDEX(pin);

    if (value)
    {
        gpio->BSRR = 1UL << index;
    }
    else
    {
        gpio->BRR = 1UL << index;
    }
}

int bsp_pin_read(bsp_pin_t pin)
{
    GPIO_TypeDef *gpio = pin_port(pin);

    return (gpio->IDR & (1UL << BSP_PIN_INDEX(pin))) ? 1 : 0;
}

void bsp_delay_us(uint32_t us)
{
    for (volatile uint32_t i = 0; i < us * 8U; i++)
    {
    }
}
