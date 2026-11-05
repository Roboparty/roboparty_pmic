#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "bsp.h"
#include "gd32f50x_rcu.h"
#include "gd32f50x_gpio.h"

static rt_uint8_t heap_buffer[16384] __attribute__((aligned(8)));

void rt_hw_board_init(void)
{
    SCB->VTOR = OTA_APP_ENTRY;
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    /* Keep the charge stage disabled through all peripheral setup. */
    gpio_bit_reset(GPIOB, GPIO_PIN_3);
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL1, GPIO_PIN_3);
    gpio_bit_reset(GPIOA, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL1,
                            GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

    rt_system_heap_init(heap_buffer, heap_buffer + sizeof(heap_buffer));
    rt_hw_serial_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
    NVIC_SetPriority(SysTick_IRQn, 15);
}

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

uint32_t bsp_tick_get(void)
{
    return (uint32_t)(rt_tick_get() * 1000U / RT_TICK_PER_SECOND);
}

void bsp_delay_ms(uint32_t ms)
{
    rt_thread_mdelay((rt_int32_t)ms);
}

int __io_putchar(int ch)
{
    rt_device_t dev = rt_device_find("uart3");
    if (dev != RT_NULL)
    {
        char c = (char)ch;
        rt_device_write(dev, 0, &c, 1);
    }
    return ch;
}

void Error_Handler(void)
{
    __disable_irq();
    for (;;)
    {
    }
}
