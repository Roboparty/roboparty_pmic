#include <rtthread.h>

#include "app_charge.h"
#include "app_config.h"
#include "app_state.h"

#include "ch224.h"
#include "bsp.h"
#include "tmp100.h"

static struct rt_semaphore adc_done;

void app_charge_init(void)
{
    rt_sem_init(&adc_done, "adc", 0, RT_IPC_FLAG_PRIO);

    bsp_pwm_init();
    bsp_adc_init();
    ApplyConfig();
}

static int adc_sample(void)
{
    if (bsp_adc_start() != 0)
    {
        return -1;
    }

    if (rt_sem_take(&adc_done, rt_tick_from_millisecond(100)) != RT_EOK)
    {
        bsp_adc_stop();
        while (rt_sem_trytake(&adc_done) == RT_EOK)
        {
        }
        return -1;
    }

    return 0;
}

void bsp_adc_complete(void)
{
    rt_sem_release(&adc_done);
}

static void charge_safety_check(void)
{
    float chrg_v;
    float bat_v;
    float chrg_i;
    float est_temp;
    uint16_t soc;
#ifndef PMIC_LEFT_LAYOUT
    uint8_t bat_inst;
    uint8_t ir_rec;
#endif
    uint8_t trip;

    app_state_lock();
    chrg_v = pEnvSecure->estimate_chrg_volt;
    bat_v = pEnvSecure->estimate_bat_volt;
    chrg_i = pEnvSecure->estimate_chrg_current;
    est_temp = pEnvSecure->estimate_temp;
    soc = BmsSoc[0];
#ifndef PMIC_LEFT_LAYOUT
    bat_inst = BAT_INST;
    ir_rec = IR_REC;
#endif
    trip = (chrg_v >= 32.0f) ||
           (chrg_v >= 5.5f && chrg_v <= 16.0f) ||
           (bat_v >= 55.0f) ||
           (est_temp >= 80.0f) ||
           (chrg_i >= 6.0f) ||
           (chrg_v >= 16.0f && soc == 100)
#ifndef PMIC_LEFT_LAYOUT
           || (bat_inst == 0 && ir_rec == 1)
#endif
           ;
    if (trip)
    {
        pEnvSecure->exception = 1;
    }
    app_state_unlock();

    if (trip)
    {
        app_config_exception_shutdown();
    }
}

void app_charge_thread_entry(void *parameter)
{
    uint8_t sample_count = 0;
    float sum_bat_v = 0.0f;
    float sum_chrg_i = 0.0f;
    float sum_chrg_v = 0.0f;
    float sum_temp = 0.0f;

    (void)parameter;

    rt_thread_mdelay(1000);

    while (1)
    {
        if (adc_sample() == 0)
        {
            float chrg_i = (float)bsp_adc_values[0] / 4095.0f * 3.3f / 0.4536f;
            float bat_v = (float)bsp_adc_values[1] / 4095.0f * 3.3f / 0.0449f;
            float chrg_v = (float)bsp_adc_values[2] / 4095.0f * 3.3f / 0.0909f;
            float temperature = TMP100_GetTemperature();

            app_state_lock();
            CHRG_I = chrg_i;
            BAT_V = bat_v;
            CHRG_V = chrg_v;
            CHRG_PWR = (uint16_t)(chrg_i * chrg_v);
            temp = temperature;
            app_state_unlock();

            sum_bat_v += bat_v;
            sum_chrg_i += chrg_i;
            sum_chrg_v += chrg_v;
            sum_temp += temperature;
            sample_count++;

            if (sample_count >= 3)
            {
                app_state_lock();
                pEnvSecure->estimate_pre_chrg_volt = pEnvSecure->estimate_chrg_volt;
                pEnvSecure->estimate_bat_volt = sum_bat_v / 3.0f;
                pEnvSecure->estimate_chrg_current = sum_chrg_i / 3.0f;
                pEnvSecure->estimate_chrg_volt = sum_chrg_v / 3.0f;
                pEnvSecure->estimate_temp = sum_temp / 3.0f;
                app_state_unlock();

                sum_bat_v = 0.0f;
                sum_chrg_i = 0.0f;
                sum_chrg_v = 0.0f;
                sum_temp = 0.0f;
                sample_count = 0;

                CH224_ReRequestPPS();
            }
        }

        float max_current = CH224_ReadCurrent() * 0.05f;

        float est_temp;
        float chrg_v;
        uint8_t exception;
        uint8_t charge_ready;

        app_state_lock();
        est_temp = pEnvSecure->estimate_temp;
        chrg_v = CHRG_V;
        exception = pEnvSecure->exception;
#ifdef PMIC_LEFT_LAYOUT
        charge_ready = 0;
#else
        charge_ready = BAT_INST;
#endif
        app_state_unlock();

        uint8_t fan;
        if (chrg_v >= 16.0f && charge_ready && exception == 0)
        {
            if (est_temp <= 50.0f)
            {
                fan = 49;
            }
            else if (est_temp <= 70.0f)
            {
                fan = (uint8_t)(49.0f + (est_temp - 50.0f) * 2.5f);
            }
            else
            {
                fan = 99;
            }
        }
        else
        {
            fan = 0;
        }
        app_config_set_fan(fan);

        uint8_t charge_enable = 0;
        uint8_t ictrl = 0;
        if (chrg_v >= 16.0f && charge_ready && exception == 0)
        {
            ictrl = (uint8_t)(max_current / 5.0f * 70.0f);
            charge_enable = 1;
        }
        app_config_set_charge(charge_enable, ictrl);

#ifdef PMIC_LEFT_LAYOUT
        bsp_pin_write(BOARD_PIN_BAT_G, charge_ready && !exception);
        bsp_pin_write(BOARD_PIN_BAT_R, exception != 0);
#else
        bsp_pin_write(BOARD_PIN_IR_EM, 1);
        rt_thread_mdelay(10);
        uint8_t ir_rec = bsp_pin_read(BOARD_PIN_IR_REC) ^ 0x01;
        bsp_pin_write(BOARD_PIN_IR_EM, 0);

        app_state_lock();
        IR_REC = ir_rec;
        app_state_unlock();
#endif

        charge_safety_check();

        rt_thread_mdelay(300);
    }
}
