#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "gd32f50x_gpio.h"
#include "gd32f50x_rcu.h"
#include "gd32f50x_usart.h"

static struct rt_serial_device host_serial;

static rt_err_t host_configure(struct rt_serial_device *serial,
                               struct serial_configure *config)
{
    (void)serial;
    usart_disable(USART0);
    usart_baudrate_set(USART0, config->baud_rate);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
    return RT_EOK;
}

static rt_err_t host_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    (void)serial;
    (void)arg;
    if (cmd == RT_DEVICE_CTRL_SET_INT)
        usart_interrupt_enable(USART0, USART_INT_RBNE);
    else if (cmd == RT_DEVICE_CTRL_CLR_INT || cmd == RT_DEVICE_CTRL_CLOSE)
        usart_interrupt_disable(USART0, USART_INT_RBNE);
    return RT_EOK;
}

static int host_putc(struct rt_serial_device *serial, char c)
{
    (void)serial;
    while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
    {
    }
    usart_data_transmit(USART0, (uint8_t)c);
    return 1;
}

static int host_getc(struct rt_serial_device *serial)
{
    (void)serial;
    if (usart_flag_get(USART0, USART_FLAG_RBNE) == RESET)
        return -1;
    return usart_data_receive(USART0) & 0xFF;
}

static rt_ssize_t host_dma_transmit(struct rt_serial_device *serial,
                                   rt_uint8_t *buf, rt_size_t size, int direction)
{
    (void)serial;
    (void)buf;
    (void)size;
    (void)direction;
    return -RT_ENOSYS;
}

static const struct rt_uart_ops host_ops = {
    .configure = host_configure,
    .control = host_control,
    .putc = host_putc,
    .getc = host_getc,
    .dma_transmit = host_dma_transmit,
};

void USART0_IRQHandler(void)
{
    rt_interrupt_enter();
    if (usart_flag_get(USART0, USART_FLAG_ORERR) == SET)
    {
        (void)usart_data_receive(USART0);
    }
    else if (usart_flag_get(USART0, USART_FLAG_RBNE) == SET)
    {
        rt_hw_serial_isr(&host_serial, RT_SERIAL_EVENT_RX_IND);
    }
    rt_interrupt_leave();
}

void rt_hw_serial_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rcu_periph_clock_enable(RCU_USART0);
    gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2,
                            GPIO_PIN_9 | GPIO_PIN_10);
    host_serial.ops = &host_ops;
    host_serial.config = config;
    rt_hw_serial_register(&host_serial, "uart3",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, RT_NULL);
    NVIC_SetPriority(USART0_IRQn, 6);
    NVIC_EnableIRQ(USART0_IRQn);
}
