#include <string.h>

#include <rtthread.h>

#include "app_config.h"

ConfigData g_config;

static struct rt_mutex config_mutex;

void app_config_lock(void)
{
    rt_mutex_take(&config_mutex, RT_WAITING_FOREVER);
}

void app_config_unlock(void)
{
    rt_mutex_release(&config_mutex);
}

void SaveConfigToFlash(ConfigData *cfg)
{
    const uint8_t *data = (const uint8_t *)cfg;

    rt_enter_critical();

    bsp_flash_erase_page(CFG_FLASH_ADDR);
    for (uint16_t i = 0; i < CFG_SAVE_BYTES; i += 2)
    {
        uint16_t word = (uint16_t)data[i] | ((i + 1U < CFG_SAVE_BYTES) ? (data[i + 1] << 8) : 0U);
        bsp_flash_write_halfword(CFG_FLASH_ADDR + i, word);
    }

    rt_exit_critical();
}

int LoadConfigFromFlash(ConfigData *cfg)
{
    const ConfigData *p = (const ConfigData *)CFG_FLASH_ADDR;

    if (p->magic == CFG_MAGIC_NUMBER)
    {
        memcpy(cfg, p, CFG_SAVE_BYTES);
        return 0;
    }

    cfg->magic = CFG_MAGIC_NUMBER;
    cfg->ictrl = 0;
    cfg->r = 0;
    cfg->g = 0;
    cfg->b = 0;
    cfg->chg_en = 0;
    cfg->fan = 49;
    return 1;
}

static void apply_config_locked(void)
{
    bsp_pwm_set(BSP_PWM_LED_R, (g_config.r * 500) / 100);
    bsp_pwm_set(BSP_PWM_LED_G, (g_config.g * 500) / 100);
    bsp_pwm_set(BSP_PWM_LED_B, (g_config.b * 500) / 100);
    bsp_pwm_set(BSP_PWM_FAN, (g_config.fan * 99) / 100);
    bsp_pwm_set(BSP_PWM_CHARGE, (g_config.ictrl * 7200) / 100);
    bsp_pin_write(BOARD_PIN_CHARGE_EN, g_config.chg_en ? 1 : 0);
}

void ApplyConfig(void)
{
    app_config_lock();
    apply_config_locked();
    app_config_unlock();
}

void app_config_set_fan(uint8_t fan)
{
    app_config_lock();
    g_config.fan = fan;
    bsp_pwm_set(BSP_PWM_FAN, (g_config.fan * 99) / 100);
    app_config_unlock();
}

void app_config_set_charge(uint8_t enable, uint8_t ictrl)
{
    app_config_lock();
    g_config.chg_en = enable ? 1 : 0;
    g_config.ictrl = ictrl;
    bsp_pwm_set(BSP_PWM_CHARGE, (g_config.ictrl * 7200) / 100);
    bsp_pin_write(BOARD_PIN_CHARGE_EN, g_config.chg_en ? 1 : 0);
    app_config_unlock();
}

void app_config_exception_shutdown(void)
{
    app_config_lock();
    g_config.chg_en = 0;
    g_config.ictrl = 0;
    g_config.r = 80;
    g_config.g = 0;
    g_config.b = 0;
    apply_config_locked();
    app_config_unlock();
}

void app_config_init(void)
{
    rt_mutex_init(&config_mutex, "cfg", RT_IPC_FLAG_PRIO);

    if (LoadConfigFromFlash(&g_config) == 1)
    {
        g_config.b = 10;
        g_config.g = 10;
        g_config.r = 10;
        g_config.chg_en = 0;
        g_config.ictrl = 0;
        g_config.fan = 49;
    }

#ifdef PMIC_LEFT_LAYOUT
    /* The charge thread owns automatic enable after startup checks. */
    g_config.chg_en = 0;
    g_config.ictrl = 0;
#endif
    ApplyConfig();
}

void UpdateConfigFromCommand(const char *cmd_name, int value)
{
#ifdef PMIC_LEFT_LAYOUT
    if (strcmp(cmd_name, "CHG_EN") == 0 || strcmp(cmd_name, "ICTRL") == 0)
    {
        return;
    }
#endif
    app_config_lock();

    if (strcmp(cmd_name, "ICTRL") == 0)
    {
        if (value >= 0 && value <= 100) g_config.ictrl = (uint8_t)value;
    }
    else if (strcmp(cmd_name, "R") == 0)
    {
        if (value >= 0 && value <= 100) g_config.r = (uint8_t)value;
    }
    else if (strcmp(cmd_name, "G") == 0)
    {
        if (value >= 0 && value <= 100) g_config.g = (uint8_t)value;
    }
    else if (strcmp(cmd_name, "B") == 0)
    {
        if (value >= 0 && value <= 100) g_config.b = (uint8_t)value;
    }
    else if (strcmp(cmd_name, "CHG_EN") == 0)
    {
        if (value == 0 || value == 1) g_config.chg_en = (uint8_t)value;
    }
    else if (strcmp(cmd_name, "FAN") == 0)
    {
        if (value >= 0 && value <= 100) g_config.fan = (uint8_t)value;
    }

    app_config_unlock();
}
