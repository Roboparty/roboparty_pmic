#include <rtthread.h>

#include "board.h"
#include "bsp.h"

volatile uint16_t bsp_adc_values[3];

__attribute__((weak)) void bsp_adc_complete(void)
{
}

void bsp_adc_init(void)
{
    ADC1->CR1 = ADC_CR1_SCAN;
    ADC1->SQR1 = (3U - 1U) << 20;
    ADC1->SQR3 = (0U << 0) | (1U << 5) | (2U << 10);
    ADC1->SMPR2 = (6U << 0) | (6U << 3) | (6U << 6);

    ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_DMA |
                  ADC_CR2_EXTSEL_2 | ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_0 |
                  ADC_CR2_EXTTRIG;
    for (volatile uint32_t i = 0; i < 1000U; i++)
    {
    }

    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0U)
    {
    }

    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0U)
    {
    }

    NVIC_SetPriority(DMA1_Channel1_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

int bsp_adc_start(void)
{
    DMA1_Channel1->CCR = 0U;
    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR = (uint32_t)bsp_adc_values;
    DMA1_Channel1->CNDTR = 3U;
    DMA1_Channel1->CCR = DMA_CCR_MINC | DMA_CCR_PSIZE_0 | DMA_CCR_MSIZE_0 |
                         DMA_CCR_TCIE | DMA_CCR_EN;

    ADC1->CR2 |= ADC_CR2_SWSTART | ADC_CR2_EXTTRIG;
    return 0;
}

void bsp_adc_stop(void)
{
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
}

void DMA1_Channel1_IRQHandler(void)
{
    rt_interrupt_enter();

    if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        DMA1_Channel1->CCR &= ~DMA_CCR_EN;
        bsp_adc_complete();
    }

    rt_interrupt_leave();
}
