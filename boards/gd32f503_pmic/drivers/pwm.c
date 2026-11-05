#include "board.h"
#include "bsp.h"
#include "gd32f50x_gpio.h"
#include "gd32f50x_rcu.h"
#include "gd32f50x_timer.h"

static void pwm_timer_init(uint32_t timer, uint32_t period, uint32_t prescaler,
                           uint16_t channel)
{
    timer_parameter_struct base;
    timer_oc_parameter_struct output;

    timer_struct_para_init(&base);
    base.prescaler = prescaler;
    base.period = period;
    base.alignedmode = TIMER_COUNTER_EDGE;
    base.counterdirection = TIMER_COUNTER_UP;
    base.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(timer, &base);

    timer_channel_output_struct_para_init(&output);
    output.outputstate = TIMER_CCX_ENABLE;
    output.ocpolarity = TIMER_OC_POLARITY_HIGH;
    timer_channel_output_config(timer, channel, &output);
    timer_channel_output_pulse_value_config(timer, channel, 0U);
    timer_channel_output_mode_config(timer, channel, TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(timer, channel, TIMER_OC_SHADOW_DISABLE);
    timer_auto_reload_shadow_enable(timer);
    timer_primary_output_config(timer, ENABLE);
    timer_enable(timer);
}

void bsp_pwm_init(void)
{
    rcu_periph_clock_enable(RCU_TIMER0);
    rcu_periph_clock_enable(RCU_TIMER16);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_8);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2,
                            GPIO_PIN_3 | GPIO_PIN_8);
    gpio_af_set(GPIOA, GPIO_AF_2, GPIO_PIN_3);
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_8);
    pwm_timer_init(TIMER0, 99U, SystemCoreClock / 10000U - 1U, TIMER_CH_0);
    pwm_timer_init(TIMER16, 7199U, 2U, TIMER_CH_1);
}

void bsp_pwm_set(int channel, uint16_t value)
{
    switch (channel)
    {
    case BSP_PWM_FAN:
        timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_0, value > 99U ? 99U : value);
        break;
    case BSP_PWM_CHARGE:
        timer_channel_output_pulse_value_config(TIMER16, TIMER_CH_1,
                                                value > 7199U ? 7199U : value);
        break;
    case BSP_PWM_LED_R:
        bsp_pin_write(BOARD_PIN_LED_R, value != 0U);
        break;
    case BSP_PWM_LED_G:
        bsp_pin_write(BOARD_PIN_LED_G, value != 0U);
        break;
    default:
        break;
    }
}
