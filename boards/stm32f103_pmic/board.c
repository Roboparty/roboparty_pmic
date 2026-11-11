#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"
#include "bsp.h"

uint32_t SystemCoreClock = 72000000U;

void SystemInit(void)
{
    SCB->VTOR = 0x08001000U;
}

static void gpio_mode(uint32_t port, uint32_t pin, uint32_t cnf, uint32_t mode)
{
    GPIO_TypeDef *gpio = (port == 0U) ? GPIOA : GPIOB;
    volatile uint32_t *reg = (pin < 8U) ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin & 0x07U) * 4U;

    *reg = (*reg & ~(0x0FU << shift)) | ((cnf << 2U | mode) << shift);
}

static void board_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_1;

    /* PA0/1/2: charge current, battery voltage, input voltage. */
#ifdef PMIC_LEFT_LAYOUT
    /* PA4/5: status LEDs; PA6/7: battery LEDs. */
    gpio_mode(0, 4, 0x0U, 0x2U);
    gpio_mode(0, 5, 0x0U, 0x2U);
    gpio_mode(0, 6, 0x0U, 0x2U);
    gpio_mode(0, 7, 0x0U, 0x2U);
#else
    /* PA4 buzzer, PA5 key. */
    gpio_mode(0, 4, 0x0U, 0x2U);
    gpio_mode(0, 5, 0x1U, 0x0U);
#endif
    gpio_mode(0, 0, 0x0U, 0x0U);
    gpio_mode(0, 1, 0x0U, 0x0U);
    gpio_mode(0, 2, 0x0U, 0x0U);

    /* PA3 TIM2_CH4 current control, PA8 TIM1_CH1 fan. */
    gpio_mode(0, 3, 0x2U, 0x3U);
#ifndef PMIC_LEFT_LAYOUT
    gpio_mode(0, 6, 0x2U, 0x3U);
    gpio_mode(0, 7, 0x2U, 0x3U);
#endif
    gpio_mode(0, 8, 0x2U, 0x3U);

    /* PA9 USART1_TX, PA10 USART1_RX: host protocol and console. */
    gpio_mode(0, 9, 0x2U, 0x3U);
    gpio_mode(0, 10, 0x1U, 0x0U);

#ifndef PMIC_LEFT_LAYOUT
    /* PB0 TIM3_CH3, PB1/PB2 OLED I2C. */
    gpio_mode(1, 0, 0x2U, 0x3U);
    gpio_mode(1, 1, 0x1U, 0x2U);
    gpio_mode(1, 2, 0x1U, 0x2U);
#endif
    /* PB3 charge enable; PB4 is only connected on the right board. */
    gpio_mode(1, 3, 0x0U, 0x2U);

#ifndef PMIC_LEFT_LAYOUT
    gpio_mode(1, 4, 0x1U, 0x0U);
    /* PB6/7 infrared pair. */
    gpio_mode(1, 6, 0x1U, 0x0U);
    gpio_mode(1, 7, 0x0U, 0x2U);
#endif

    /* PB8/PB9 CH224/TMP100 I2C */
    gpio_mode(1, 8, 0x1U, 0x2U);
    gpio_mode(1, 9, 0x1U, 0x2U);

#ifndef PMIC_LEFT_LAYOUT
    /* PB10 USART3_TX, PB11 USART3_RX. */
    gpio_mode(1, 10, 0x2U, 0x3U);
    gpio_mode(1, 11, 0x1U, 0x0U);

    /* PB12 SPI CS. */
    gpio_mode(1, 12, 0x0U, 0x2U);
    GPIOB->BSRR = GPIO_BSRR_BS12;
#endif

#ifdef PMIC_LEFT_LAYOUT
    GPIOA->BRR = GPIO_BRR_BR4 | GPIO_BRR_BR5 | GPIO_BRR_BR6 | GPIO_BRR_BR7;
#else
    GPIOA->BRR = GPIO_BRR_BR4;
    GPIOB->BRR = GPIO_BRR_BR7;
#endif
    GPIOB->BRR = GPIO_BRR_BR3;
}

void SystemClock_Config(void)
{
    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0U)
    {
    }

    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL |
                               RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2))
              | RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9
              | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
    {
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV6;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN |
                    RCC_APB1ENR_PWREN;
#ifndef PMIC_LEFT_LAYOUT
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
#endif
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_TIM1EN |
                    RCC_APB2ENR_USART1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    SystemCoreClock = 72000000U;
}

uint32_t bsp_tick_get(void)
{
    return (uint32_t)(rt_tick_get() * 1000U / RT_TICK_PER_SECOND);
}

void bsp_delay_ms(uint32_t ms)
{
    rt_thread_mdelay((rt_int32_t)ms);
}

static rt_uint8_t heap_buffer[8192] __attribute__((aligned(8)));

void rt_hw_board_init(void)
{
    SystemClock_Config();
    board_gpio_init();

    rt_system_heap_init(heap_buffer, heap_buffer + sizeof(heap_buffer));

    rt_hw_serial_init();
    rt_hw_pin_init();

#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
    NVIC_SetPriority(SysTick_IRQn, 15);
}

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
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
    rt_hw_interrupt_disable();
    while (1)
    {
    }
}
