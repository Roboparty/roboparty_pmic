#include <stdio.h>

#include <rtthread.h>

#include "app_ui.h"
#include "app_state.h"

#include "key.h"
#include "oled.h"

static int digit_index(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    return 11;
}

static void draw_digit_12(uint8_t x, uint8_t y, char c)
{
    OLED_DrawBMP(x, y, x + 16, y + 1, &gImage_12x12[digit_index(c)][0]);
}

static void draw_digit_bw(uint8_t x, uint8_t y, char c)
{
    OLED_DrawBMP(x, y, x + 16, y + 1, &gImage_BBWW[digit_index(c)][0]);
}

static void lcd_disp_refresh(void)
{
    char voltage[16];
    char current[16];
    char temperature[16];
    char soc[6];
    char power[8];
    float chrg_v;
    float chrg_i;
    float battery_v;
    float temp_value;
    uint16_t soc_value;
    uint16_t power_value;
    uint8_t exception;

    app_state_lock();
    chrg_v = CHRG_V;
    chrg_i = CHRG_I;
    battery_v = BAT_V;
    temp_value = temp;
    soc_value = BmsSoc[0];
    power_value = CHRG_PWR;
    app_state_unlock();

    sprintf(voltage, "%-4.1f", chrg_v);
    sprintf(current, "%-3.1f", chrg_i);
    sprintf(temperature, "%-4.1f", temp_value);
    sprintf(soc, "%d", soc_value);
    sprintf(power, "%d", power_value);

    if (KEY1_EVENT)
    {
        KEY1_EVENT = 0;
        KEY1_VAL = !KEY1_VAL;
        OLED_Clear();
        OLED_DrawBMP(0, 0, 128, 8, KEY1_VAL ? gImage_SUM_INFO : gImage_PD_PWR);
    }
    else if (KEY2_EVENT)
    {
        KEY2_EVENT = 0;
        KEY2_VAL = !KEY2_VAL;
        OLED_SetDirection(KEY2_VAL);
        OLED_Clear();
        OLED_DrawBMP(0, 0, 128, 8, KEY1_VAL ? gImage_SUM_INFO : gImage_PD_PWR);
    }

    if (KEY_LONG_EVENT)
    {
        KEY_LONG_EVENT = 0;
        CLEAN_EXCP = 0;
        KEY1_VAL = 1;

        app_state_lock();
        pEnvSecure->exception = 0;
        app_state_unlock();

        OLED_Clear();
        OLED_DrawBMP(0, 0, 128, 8, gImage_SUM_INFO);
    }
    app_state_lock();
    exception = pEnvSecure->exception;
    app_state_unlock();

    if (exception)
    {
        char line[16];

        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t *)"Exception!", 16);
        OLED_ShowString(0, 2, (uint8_t *)"SoC:", 16);
        OLED_ShowString(32, 2, (uint8_t *)soc, 16);

        sprintf(line, "CI:%-3.1fA", chrg_i);
        OLED_ShowString(72, 4, (uint8_t *)line, 16);
        sprintf(line, "CV:%-4.1fV", chrg_v);
        OLED_ShowString(0, 4, (uint8_t *)line, 16);
        sprintf(line, "BV:%-4.1fV", battery_v);
        OLED_ShowString(0, 6, (uint8_t *)line, 16);
        sprintf(line, "T:%-4.1f", temp_value);
        OLED_ShowString(72, 6, (uint8_t *)line, 16);
        return;
    }

    if (KEY1_VAL)
    {
        if (voltage[2] == '.')
        {
            draw_digit_12(19, 6, voltage[0]);
            draw_digit_12(19, 5, voltage[1]);
            draw_digit_12(19, 4, '.');
            draw_digit_12(19, 3, voltage[3]);
        }
        else
        {
            draw_digit_12(19, 6, ' ');
            draw_digit_12(19, 5, voltage[0]);
            draw_digit_12(19, 4, '.');
            draw_digit_12(19, 3, voltage[2]);
        }

        if (current[1] == '.')
        {
            draw_digit_12(49, 5, current[0]);
            draw_digit_12(49, 4, '.');
            draw_digit_12(49, 3, current[2]);
        }
        else
        {
            draw_digit_12(49, 5, current[0]);
            draw_digit_12(49, 4, current[1]);
            draw_digit_12(49, 3, current[2]);
        }

        if (temperature[2] == '.')
        {
            draw_digit_bw(98, 6, temperature[0]);
            draw_digit_bw(98, 5, temperature[1]);
            draw_digit_bw(98, 4, '.');
            draw_digit_bw(98, 3, temperature[3]);
        }
        else
        {
            draw_digit_bw(98, 6, ' ');
            draw_digit_bw(98, 5, temperature[0]);
            draw_digit_bw(98, 4, '.');
            draw_digit_bw(98, 3, temperature[2]);
        }
    }
    else
    {
        if (soc_value == 100)
        {
            draw_digit_bw(22, 5, soc[0]);
            draw_digit_bw(22, 4, soc[1]);
            draw_digit_bw(22, 3, soc[2]);
        }
        else
        {
            draw_digit_bw(22, 5, ' ');
            draw_digit_bw(22, 4, soc[0]);
            draw_digit_bw(22, 3, soc[1]);
        }

        if (power_value / 100)
        {
            draw_digit_12(96, 5, power[0]);
            draw_digit_12(96, 4, power[1]);
            draw_digit_12(96, 3, power[2]);
        }
        else if (power_value / 10)
        {
            draw_digit_12(96, 5, ' ');
            draw_digit_12(96, 4, power[0]);
            draw_digit_12(96, 3, power[1]);
        }
        else
        {
            draw_digit_12(96, 5, ' ');
            draw_digit_12(96, 4, ' ');
            draw_digit_12(96, 3, power[0]);
        }
    }

}

void app_ui_thread_entry(void *parameter)
{
    uint8_t refresh_count = 0;
    uint8_t last_exception = 0;
    uint32_t beep_until = 0;

    (void)parameter;

    while (1)
    {
        uint8_t exception;

        KEY_Scan();
        BAT_INST = bsp_pin_read(BOARD_PIN_BAT_INST) ^ 0x01;

        app_state_lock();
        exception = pEnvSecure->exception;
        app_state_unlock();

        if (exception && !last_exception)
        {
            bsp_pin_write(BOARD_PIN_BUZZER, 1);
            beep_until = bsp_tick_get() + 1500;
        }
        if (beep_until != 0 && (int32_t)(bsp_tick_get() - beep_until) >= 0)
        {
            bsp_pin_write(BOARD_PIN_BUZZER, 0);
            beep_until = 0;
        }
        last_exception = exception;

        refresh_count++;
        if (refresh_count >= 20 || KEY1_EVENT || KEY2_EVENT || KEY_LONG_EVENT)
        {
            refresh_count = 0;
            lcd_disp_refresh();
        }

        rt_thread_mdelay(10);
    }
}
