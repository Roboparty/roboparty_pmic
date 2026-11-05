#include "bsp.h"
#include "gd32f50x_adc.h"
#include "gd32f50x_rcu.h"

volatile uint16_t bsp_adc_values[3];

void bsp_adc_init(void)
{
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);
    adc_sync_mode_config(ADC_MODE_FREE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1U);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(ADC0);
    bsp_delay_ms(1);
}

int bsp_adc_start(void)
{
    for (uint8_t channel = 0; channel < 3U; ++channel)
    {
        uint32_t timeout = 100000U;
        adc_flag_clear(ADC0, ADC_FLAG_EORC);
        adc_routine_channel_config(ADC0, 0U, channel, ADC_SAMPLETIME_55POINT5);
        adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
        while (adc_flag_get(ADC0, ADC_FLAG_EORC) == RESET && --timeout != 0U)
        {
        }
        if (timeout == 0U) return -1;
        bsp_adc_values[channel] = adc_routine_data_read(ADC0);
    }
    bsp_adc_complete();
    return 0;
}

void bsp_adc_stop(void)
{
}
