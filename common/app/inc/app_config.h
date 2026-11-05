#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "app_types.h"

#define CFG_FLASH_ADDR   0x0800FC00U
#define CFG_MAGIC_NUMBER 0xAA55U
#define CFG_SAVE_BYTES   36U

typedef struct
{
    uint16_t magic;
    uint8_t ictrl;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t chg_en;
    uint8_t fan;
    char str_sw_date[12];
    char str_chrg_sn[16];
} ConfigData;

extern ConfigData g_config;

void app_config_init(void);
int LoadConfigFromFlash(ConfigData *cfg);
void SaveConfigToFlash(ConfigData *cfg);
void ApplyConfig(void);
void UpdateConfigFromCommand(const char *cmd_name, int value);
void app_config_set_fan(uint8_t fan);
void app_config_set_charge(uint8_t enable, uint8_t ictrl);
#ifdef PMIC_LEFT_LAYOUT
void app_config_set_status_leds(uint8_t green, uint8_t red);
#endif
void app_config_exception_shutdown(void);
void app_config_lock(void);
void app_config_unlock(void);

#endif /* __APP_CONFIG_H__ */
