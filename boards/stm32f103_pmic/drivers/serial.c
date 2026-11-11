#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"

struct bsp_uart
{
    struct rt_serial_device serial;
    USART_TypeDef *instance;
    IRQn_Type irq;
    uint32_t pclk;
};

#ifndef PMIC_LEFT_LAYOUT
static struct bsp_uart uart1_dev;
#endif
static struct bsp_uart uart3_dev;

static rt_err_t uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct bsp_uart *uart = (struct bsp_uart *)serial;

    uart->instance->CR1 = 0U;
    uart->instance->CR2 = 0U;
    uart->instance->CR3 = 0U;

    uart->instance->BRR = (uart->pclk + cfg->baud_rate / 2U) / cfg->baud_rate;
    uart->instance->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    return RT_EOK;
}

static rt_err_t uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct bsp_uart *uart = (struct bsp_uart *)serial;

    (void)arg;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_SET_INT:
        uart->instance->CR1 |= USART_CR1_RXNEIE;
        return RT_EOK;
    case RT_DEVICE_CTRL_CLR_INT:
        uart->instance->CR1 &= ~USART_CR1_RXNEIE;
        return RT_EOK;
    case RT_DEVICE_CTRL_CLOSE:
        uart->instance->CR1 = 0U;
        return RT_EOK;
    default:
        return RT_EOK;
    }
}

static int uart_putc(struct rt_serial_device *serial, char c)
{
    struct bsp_uart *uart = (struct bsp_uart *)serial;

    while ((uart->instance->SR & USART_SR_TXE) == 0U)
    {
    }
    uart->instance->DR = (uint8_t)c;
    return 1;
}

static int uart_getc(struct rt_serial_device *serial)
{
    struct bsp_uart *uart = (struct bsp_uart *)serial;

    if ((uart->instance->SR & USART_SR_RXNE) == 0U)
    {
        return -1;
    }
    return (int)(uart->instance->DR & 0xFFU);
}

static rt_ssize_t uart_dma_transmit(struct rt_serial_device *serial,
                                    rt_uint8_t *buf, rt_size_t size, int direction)
{
    (void)serial;
    (void)buf;
    (void)size;
    (void)direction;
    return -RT_ENOSYS;
}

static const struct rt_uart_ops uart_ops =
{
    .configure = uart_configure,
    .control = uart_control,
    .putc = uart_putc,
    .getc = uart_getc,
    .dma_transmit = uart_dma_transmit,
};

static void uart_irq_handler(struct bsp_uart *uart)
{
    uint32_t sr = uart->instance->SR;

    if ((sr & (USART_SR_RXNE | USART_SR_ORE)) != 0U)
    {
        if ((sr & USART_SR_ORE) != 0U)
        {
            (void)uart->instance->SR;
            (void)uart->instance->DR;
        }
        else
        {
            rt_hw_serial_isr(&uart->serial, RT_SERIAL_EVENT_RX_IND);
        }
    }
}

void USART1_IRQHandler(void)
{
    rt_interrupt_enter();
#ifdef PMIC_LEFT_LAYOUT
    uart_irq_handler(&uart3_dev);
#else
    uart_irq_handler(&uart1_dev);
#endif
    rt_interrupt_leave();
}

#ifndef PMIC_LEFT_LAYOUT
void USART3_IRQHandler(void)
{
    rt_interrupt_enter();
    uart_irq_handler(&uart3_dev);
    rt_interrupt_leave();
}
#endif

static void uart_init_device(struct bsp_uart *uart, USART_TypeDef *instance,
                             IRQn_Type irq, uint32_t pclk, const char *name)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    uart->instance = instance;
    uart->irq = irq;
    uart->pclk = pclk;
    uart->serial.ops = &uart_ops;
    uart->serial.config = config;

    rt_hw_serial_register(&uart->serial, name,
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, RT_NULL);
}

void rt_hw_serial_init(void)
{
#ifdef PMIC_LEFT_LAYOUT
    /* Host protocol and console are routed to USART1 (PA9/PA10). */
    uart_init_device(&uart3_dev, USART1, USART1_IRQn, 72000000U, "uart3");
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);
#else
    uart_init_device(&uart1_dev, USART1, USART1_IRQn, 72000000U, "uart1");
    uart_init_device(&uart3_dev, USART3, USART3_IRQn, 36000000U, "uart3");
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART3_IRQn, 1);
    NVIC_EnableIRQ(USART3_IRQn);
#endif
}
