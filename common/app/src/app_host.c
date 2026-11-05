#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rthw.h>
#include <rtthread.h>

#include "app_host.h"
#include "app_main.h"
#include "app_config.h"
#include "app_state.h"

#include <rtdevice.h>
#include "bsp.h"
#ifdef STM32F103xB
#include "stm32f103xb.h"
#endif
#ifndef PMIC_LEFT_LAYOUT
#include "ota.h"
#include "ota_storage.h"
#endif

#define HOST_RX_SIZE     512

#ifdef PMIC_LEFT_LAYOUT
#define PMIC_FW_VERSION "0.1.7-left"
#elif defined(STM32F103xB)
#define PMIC_FW_VERSION "0.1.2-right"
#else
#define PMIC_FW_VERSION "unknown"
#endif

static rt_device_t host_dev;

static void print_serial_number(void)
{
#ifdef STM32F103xB
    const volatile uint32_t *uid = (const volatile uint32_t *)UID_BASE;
    printf("CHRG_SN:%08lX%08lX%08lX;",
           (unsigned long)uid[0], (unsigned long)uid[1],
           (unsigned long)uid[2]);
#else
    app_config_lock();
    printf("CHRG_SN:%.16s;", g_config.str_chrg_sn);
    app_config_unlock();
#endif
}

#ifndef PMIC_LEFT_LAYOUT
static uint32_t ota_size;
static uint32_t ota_expected_crc;
static uint32_t ota_calc_crc;
static uint32_t ota_offset;
static uint8_t  ota_active;

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int hex_decode(const char *hex, uint8_t *out, int max)
{
    int count = 0;

    while (hex[0] != '\0' && hex[1] != '\0' && count < max)
    {
        int hi = hex_value(hex[0]);
        int lo = hex_value(hex[1]);
        if (hi < 0 || lo < 0) break;
        out[count++] = (uint8_t)((hi << 4) | lo);
        hex += 2;
    }
    return count;
}
#endif

void app_host_init(void)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

    host_dev = rt_device_find("uart3");
    if (host_dev == RT_NULL)
    {
        return;
    }

    cfg.baud_rate = BAUD_RATE_115200;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    cfg.parity = PARITY_NONE;

    rt_device_control(host_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
    rt_device_open(host_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

#ifndef PMIC_LEFT_LAYOUT
    ota_storage_init();
#endif
}

static void parse_host_command(const uint8_t *cmd, uint16_t len)
{
    const uint8_t *p = cmd;
    const uint8_t *end = cmd + len;
    const uint8_t *start_cmd;
    const uint8_t *colon;
    const uint8_t *semicolon;
    char cmd_name[64];
    char data_str[512];

    while (p < end)
    {
        while (p < end && (*p == ' ' || *p == '\t'))
        {
            p++;
        }
        if (p >= end)
        {
            break;
        }

        start_cmd = p;
        colon = memchr(p, ':', end - p);
        if (colon == NULL)
        {
            break;
        }

        semicolon = memchr(colon, ';', end - colon);
        if (semicolon == NULL)
        {
            semicolon = end;
        }

        size_t name_len = colon - start_cmd;
        if (name_len >= sizeof(cmd_name))
        {
            name_len = sizeof(cmd_name) - 1;
        }
        memcpy(cmd_name, start_cmd, name_len);
        cmd_name[name_len] = '\0';

        const uint8_t *data_start = colon + 1;
        size_t data_len = semicolon - data_start;
        if (data_len >= sizeof(data_str))
        {
            data_len = sizeof(data_str) - 1;
        }
        memcpy(data_str, data_start, data_len);
        data_str[data_len] = '\0';

#ifndef PMIC_LEFT_LAYOUT
        if (strcmp(cmd_name, "OTA_BEGIN") == 0)
        {
            unsigned long size = 0;
            unsigned long crc = 0;
            ota_header_t header;

            if (sscanf(data_str, "%lu,%lu", &size, &crc) == 2 &&
                size > 0U && size <= OTA_MAX_SIZE)
            {
                ota_size = (uint32_t)size;
                ota_expected_crc = (uint32_t)crc;
                ota_calc_crc = OTA_CRC32_INIT;
                ota_offset = 0U;
                ota_active = 1U;

                ota_storage_erase(0U, OTA_PAYLOAD_OFFSET + ota_size);
                ota_header_fill(&header, ota_size, ota_expected_crc,
                                OTA_APP_ENTRY, 1U);
                ota_storage_write(0U, (const uint8_t *)&header, sizeof(header));
                printf("OTA_READY;");
            }
            else
            {
                printf("OTA_ERR;");
            }
        }
        else if (strcmp(cmd_name, "OTA_DATA") == 0)
        {
            char *comma = strchr(data_str, ',');
            uint8_t buffer[128];
            int count = 0;

            if (ota_active && comma != NULL)
            {
                uint32_t offset = (uint32_t)strtoul(data_str, NULL, 0);
                count = hex_decode(comma + 1, buffer, sizeof(buffer));

                if (count > 0 && (offset + (uint32_t)count) <= ota_size)
                {
                    ota_storage_write(OTA_PAYLOAD_OFFSET + offset, buffer, (uint32_t)count);
                    ota_calc_crc = ota_crc32_update(ota_calc_crc, buffer, (uint32_t)count);
                    ota_offset = offset + (uint32_t)count;
                    printf("OTA_ACK:%lu;", (unsigned long)ota_offset);
                }
                else
                {
                    printf("OTA_ERR;");
                }
            }
            else
            {
                printf("OTA_ERR;");
            }
        }
        else if (strcmp(cmd_name, "OTA_END") == 0)
        {
            if (ota_active && ota_offset == ota_size &&
                ota_crc32_final(ota_calc_crc) == ota_expected_crc)
            {
                printf("OTA_DONE;");
                rt_thread_mdelay(100);
                rt_hw_cpu_reset();
            }
            else
            {
                ota_storage_invalidate();
                ota_active = 0U;
                printf("OTA_ERR;");
            }
        }
        else if (strcmp(cmd_name, "OTA_ABORT") == 0)
        {
            ota_storage_invalidate();
            ota_active = 0U;
            printf("OTA_ABORTED;");
        }
#else
        if (strncmp(cmd_name, "OTA_", 4) == 0)
        {
            printf("OTA_ERR;");
        }
#endif
        else if (strcmp(cmd_name, "ICTRL") == 0 ||
            strcmp(cmd_name, "R") == 0 ||
            strcmp(cmd_name, "G") == 0 ||
            strcmp(cmd_name, "B") == 0 ||
            strcmp(cmd_name, "FAN") == 0 ||
            strcmp(cmd_name, "CHG_EN") == 0)
        {
            int value = atoi(data_str);
            UpdateConfigFromCommand(cmd_name, value);
            ApplyConfig();
        }
        else if (strcmp(cmd_name, "SAVE") == 0)
        {
            if (atoi(data_str) == 1)
            {
                app_config_lock();
                SaveConfigToFlash(&g_config);
                app_config_unlock();
            }
        }
        else if (strcmp(cmd_name, "CHRG_INFO") == 0)
        {
            ConfigData cfg;
            uint8_t bat_inst;
            uint8_t ir_rec;
            float chrg_v;
            float chrg_i;
            float bat_v;
            uint16_t soc;
            uint16_t soh;

            app_config_lock();
            cfg = g_config;
            app_config_unlock();

            app_state_lock();
            bat_inst = BAT_INST;
            ir_rec = IR_REC;
            chrg_v = CHRG_V;
            chrg_i = CHRG_I;
            bat_v = BAT_V;
            soc = BmsSoc[0];
            soh = BmsSoh[0];
            app_state_unlock();

            print_serial_number();
            printf("SW_DATE:%.12s;", cfg.str_sw_date);
            printf("FW_VER:%s;", PMIC_FW_VERSION);
            printf("R:%d;G:%d;B:%d;FAN:%d;", cfg.r, cfg.g, cfg.b, cfg.fan);
            printf("BAT_INST:%d;", bat_inst);
            printf("IR_REC:%d;", ir_rec);
            printf("CHRG_V:%.1fV;", chrg_v);
            printf("CHRG_I:%.1fA;", chrg_i);
            printf("BAT_V:%.1fV;", bat_v);
            printf("ICTRL:%d;", cfg.ictrl);
            printf("CHG_EN:%d;", cfg.chg_en);
            if (bat_inst)
            {
                printf("BMS_SOC:%d;", soc);
                printf("BMS_SOH:%d;", soh);
            }
        }
        else if (strcmp(cmd_name, "SN_GET") == 0)
        {
            print_serial_number();
        }
        else if (strcmp(cmd_name, "FW_VER") == 0)
        {
            printf("FW_VER:%s;", PMIC_FW_VERSION);
        }
        else if (strcmp(cmd_name, "SW_DATE") == 0)
        {
            app_config_lock();
            memset(g_config.str_sw_date, 0, sizeof(g_config.str_sw_date));
            strncpy(g_config.str_sw_date, data_str, sizeof(g_config.str_sw_date));
            app_config_unlock();
        }
        else if (strcmp(cmd_name, "CHRG_SN") == 0)
        {
#ifdef STM32F103xB
            printf("SN_READ_ONLY;");
#else
            app_config_lock();
            memset(g_config.str_chrg_sn, 0, sizeof(g_config.str_chrg_sn));
            strncpy(g_config.str_chrg_sn, data_str, sizeof(g_config.str_chrg_sn));
            app_config_unlock();
#endif
        }
        else if (strcmp(cmd_name, "BAT_INST") == 0)
        {
            printf("BAT_INST:%d;", BAT_INST);
        }
        else if (strcmp(cmd_name, "IR_REC") == 0)
        {
            printf("IR_REC:%d;", IR_REC);
        }
        else if (strcmp(cmd_name, "BAT_SOC") == 0)
        {
#ifdef PMIC_LEFT_LAYOUT
            printf("BMS_DISABLED;");
#else
            printf("BMS_SOC:%d;", BmsSoc[0]);
#endif
        }
        else if (strcmp(cmd_name, "BAT_SOH") == 0)
        {
#ifdef PMIC_LEFT_LAYOUT
            printf("BMS_DISABLED;");
#else
            printf("BMS_SOH:%d;", BmsSoh[0]);
#endif
        }
        else if (strcmp(cmd_name, "SYS_INFO") == 0)
        {
            app_diag_print();
        }

        if (strncmp(cmd_name, "OTA_", 4) != 0)
        {
            rt_thread_mdelay(100);
        }

        p = semicolon;
        if (p < end && *p == ';')
        {
            p++;
        }
    }
}

void app_host_thread_entry(void *parameter)
{
    uint8_t frame[HOST_RX_SIZE];
    uint16_t length = 0;
    uint32_t last_tick = bsp_tick_get();

    (void)parameter;

    while (1)
    {
        uint8_t ch;
        int count = 0;

        if (host_dev != RT_NULL)
        {
            count = (int)rt_device_read(host_dev, 0, &ch, 1);
        }

        if (count > 0)
        {
            if (length < sizeof(frame))
            {
                frame[length++] = ch;
            }
            last_tick = bsp_tick_get();

            if (ch == ';')
            {
                parse_host_command(frame, length);
                length = 0;
            }
        }
        else
        {
            if (length > 0 && (bsp_tick_get() - last_tick) >= 50U)
            {
                parse_host_command(frame, length);
                length = 0;
            }
            rt_thread_mdelay(1);
        }

        if (length >= sizeof(frame))
        {
            length = 0;
        }
    }
}
