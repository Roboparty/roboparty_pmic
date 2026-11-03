#include "board.h"
#include "bsp.h"
#include "pinout.h"

void bsp_pwm_init(void)
{
    /* TIM1_CH1 (PA8): fan, 100 Hz */
    TIM1->PSC = 7199U;
    TIM1->ARR = 99U;
    TIM1->CCMR1 = (6U << 4) | (1U << 3);
    TIM1->CCER = 1U;
    TIM1->BDTR = 1U << 15;
    TIM1->CR1 = (1U << 7) | (1U << 0);

    /* TIM2_CH4 (PA3): charge current DAC, 10 kHz */
    TIM2->PSC = 0U;
    TIM2->ARR = 7199U;
    TIM2->CCMR2 = (6U << 12) | (1U << 11);
    TIM2->CCER = 1U << 12;
    TIM2->CR1 = 1U;

#ifndef PMIC_LEFT_LAYOUT
    /* TIM3_CH1/2/3 (PA6/PA7/PB0): RGB LED */
    TIM3->PSC = 0U;
    TIM3->ARR = 500U;
    TIM3->CCMR1 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11);
    TIM3->CCMR2 = (6U << 4) | (1U << 3);
    TIM3->CCER = (1U << 0) | (1U << 4) | (1U << 8);
    TIM3->CR1 = 1U;
#endif
}

void bsp_pwm_set(int channel, uint16_t value)
{
    switch (channel)
    {
    case BSP_PWM_FAN:
        TIM1->CCR1 = value;
        break;
    case BSP_PWM_CHARGE:
        TIM2->CCR4 = value;
        break;
    case BSP_PWM_LED_R:
#ifdef PMIC_LEFT_LAYOUT
        bsp_pin_write(BOARD_PIN_LED_R, value != 0U);
#else
        TIM3->CCR2 = value;
#endif
        break;
    case BSP_PWM_LED_G:
#ifdef PMIC_LEFT_LAYOUT
        bsp_pin_write(BOARD_PIN_LED_G, value != 0U);
#else
        TIM3->CCR3 = value;
#endif
        break;
    case BSP_PWM_LED_B:
#ifndef PMIC_LEFT_LAYOUT
        TIM3->CCR1 = value;
#else
        (void)value;
#endif
        break;
    default:
        break;
    }
}
