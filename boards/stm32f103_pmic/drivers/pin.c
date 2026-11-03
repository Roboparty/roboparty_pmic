#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"
#include "bsp.h"

static GPIO_TypeDef *pin_port(rt_base_t pin)
{
    return (BSP_PIN_PORT(pin) == 0U) ? GPIOA : GPIOB;
}

static void pin_mode(struct rt_device *device, rt_base_t pin, rt_uint8_t mode)
{
    GPIO_TypeDef *gpio = pin_port(pin);
    uint32_t index = BSP_PIN_INDEX(pin);
    volatile uint32_t *reg = (index < 8U) ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (index & 0x07U) * 4U;
    uint32_t value;

    (void)device;

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        value = 0x2U;
        break;
    case PIN_MODE_OUTPUT_OD:
        value = 0x6U;
        break;
    case PIN_MODE_INPUT:
        value = 0x4U;
        break;
    case PIN_MODE_INPUT_PULLUP:
        value = 0x8U;
        gpio->BSRR = 1UL << index;
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        value = 0x8U;
        gpio->BRR = 1UL << index;
        break;
    default:
        value = 0x4U;
        break;
    }

    *reg = (*reg & ~(0x0FU << shift)) | (value << shift);
}

static void pin_write(struct rt_device *device, rt_base_t pin, rt_uint8_t value)
{
    GPIO_TypeDef *gpio = pin_port(pin);

    (void)device;

    if (value)
    {
        gpio->BSRR = 1UL << BSP_PIN_INDEX(pin);
    }
    else
    {
        gpio->BRR = 1UL << BSP_PIN_INDEX(pin);
    }
}

static rt_ssize_t pin_read(struct rt_device *device, rt_base_t pin)
{
    GPIO_TypeDef *gpio = pin_port(pin);

    (void)device;

    return (gpio->IDR & (1UL << BSP_PIN_INDEX(pin))) ? PIN_HIGH : PIN_LOW;
}

static rt_err_t pin_attach_irq(struct rt_device *device, rt_base_t pin,
                               rt_uint8_t mode, void (*hdr)(void *args), void *args)
{
    (void)device;
    (void)pin;
    (void)mode;
    (void)hdr;
    (void)args;
    return -RT_ENOSYS;
}

static rt_err_t pin_detach_irq(struct rt_device *device, rt_base_t pin)
{
    (void)device;
    (void)pin;
    return -RT_ENOSYS;
}

static rt_err_t pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint8_t enabled)
{
    (void)device;
    (void)pin;
    (void)enabled;
    return -RT_ENOSYS;
}

static rt_base_t pin_get(const char *name)
{
    (void)name;
    return -1;
}

static rt_err_t pin_debounce(struct rt_device *device, rt_base_t pin, rt_uint32_t debounce)
{
    (void)device;
    (void)pin;
    (void)debounce;
    return -RT_ENOSYS;
}

static const struct rt_pin_ops pin_ops =
{
    .pin_mode = pin_mode,
    .pin_write = pin_write,
    .pin_read = pin_read,
    .pin_attach_irq = pin_attach_irq,
    .pin_detach_irq = pin_detach_irq,
    .pin_irq_enable = pin_irq_enable,
    .pin_get = pin_get,
    .pin_debounce = pin_debounce,
};

void rt_hw_pin_init(void)
{
    rt_device_pin_register("gpio", &pin_ops, RT_NULL);
}
