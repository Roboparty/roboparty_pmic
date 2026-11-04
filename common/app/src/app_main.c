#include <stdio.h>
#include <string.h>

#include <rtthread.h>

#include "app_main.h"
#ifndef PMIC_LEFT_LAYOUT
#include "app_bms.h"
#endif
#include "app_charge.h"
#include "app_config.h"
#include "app_host.h"
#include "app_state.h"
#include "app_ui.h"
#include "bsp.h"

#include "i2c_soft.h"
#include "oled.h"

#define CHARGE_THREAD_PRIORITY  5
#ifndef PMIC_LEFT_LAYOUT
#define BMS_THREAD_PRIORITY     6
#endif
#define UI_THREAD_PRIORITY      8
#define HOST_THREAD_PRIORITY    9

#define CHARGE_STACK_SIZE       1024
#ifndef PMIC_LEFT_LAYOUT
#define BMS_STACK_SIZE          1024
#endif
#define UI_STACK_SIZE           1280
#define HOST_STACK_SIZE         2560

static struct rt_thread charge_thread;
#ifndef PMIC_LEFT_LAYOUT
static struct rt_thread bms_thread;
static struct rt_thread ui_thread;
#endif
static struct rt_thread host_thread;

static rt_uint8_t charge_stack[CHARGE_STACK_SIZE] __attribute__((aligned(8)));
#ifndef PMIC_LEFT_LAYOUT
static rt_uint8_t bms_stack[BMS_STACK_SIZE] __attribute__((aligned(8)));
static rt_uint8_t ui_stack[UI_STACK_SIZE] __attribute__((aligned(8)));
#endif
static rt_uint8_t host_stack[HOST_STACK_SIZE] __attribute__((aligned(8)));

static rt_uint32_t stack_used(const rt_uint8_t *stack, rt_uint32_t size)
{
    rt_uint32_t offset;

    for (offset = 0; offset < size; offset++)
    {
        if (stack[offset] != '#')
        {
            break;
        }
    }

    return size - offset;
}

static void stack_report(const char *name, const rt_uint8_t *stack, rt_uint32_t size)
{
    printf("%s_STACK:%lu/%lu;", name,
           (unsigned long)stack_used(stack, size),
           (unsigned long)size);
}

void app_diag_print(void)
{
    printf("UPTIME:%lu;", (unsigned long)(rt_tick_get() * 1000UL / RT_TICK_PER_SECOND));
    stack_report("CHARGE", charge_stack, sizeof(charge_stack));
#ifndef PMIC_LEFT_LAYOUT
    stack_report("BMS", bms_stack, sizeof(bms_stack));
    stack_report("UI", ui_stack, sizeof(ui_stack));
#endif
    stack_report("HOST", host_stack, sizeof(host_stack));
}

int app_main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    memset(charge_stack, 0xA5, sizeof(charge_stack));
#ifndef PMIC_LEFT_LAYOUT
    memset(bms_stack, 0xA5, sizeof(bms_stack));
    memset(ui_stack, 0xA5, sizeof(ui_stack));
#endif
    memset(host_stack, 0xA5, sizeof(host_stack));

    app_state_init();
    app_config_init();

    rt_thread_mdelay(1000);

    I2C_SOFT_INIT();
#ifndef PMIC_LEFT_LAYOUT
    OLED_Init();
    OLED_DrawBMP(0, 0, 128, 8, gImage_SUM_INFO);
#endif

    app_charge_init();
    app_host_init();

    if (rt_thread_init(&charge_thread, "charge", app_charge_thread_entry, RT_NULL,
                       charge_stack, sizeof(charge_stack), CHARGE_THREAD_PRIORITY, 10) != RT_EOK)
    {
        Error_Handler();
    }
#ifndef PMIC_LEFT_LAYOUT
    if (rt_thread_init(&bms_thread, "bms", app_bms_thread_entry, RT_NULL,
                       bms_stack, sizeof(bms_stack), BMS_THREAD_PRIORITY, 10) != RT_EOK)
    {
        Error_Handler();
    }
    if (rt_thread_init(&ui_thread, "ui", app_ui_thread_entry, RT_NULL,
                       ui_stack, sizeof(ui_stack), UI_THREAD_PRIORITY, 10) != RT_EOK)
    {
        Error_Handler();
    }
#endif
    if (rt_thread_init(&host_thread, "host", app_host_thread_entry, RT_NULL,
                       host_stack, sizeof(host_stack), HOST_THREAD_PRIORITY, 10) != RT_EOK)
    {
        Error_Handler();
    }

    if (rt_thread_startup(&charge_thread) != RT_EOK ||
#ifndef PMIC_LEFT_LAYOUT
        rt_thread_startup(&bms_thread) != RT_EOK ||
        rt_thread_startup(&ui_thread) != RT_EOK ||
#endif
        rt_thread_startup(&host_thread) != RT_EOK)
    {
        Error_Handler();
    }

    while (1)
    {
        rt_thread_mdelay(1000);
    }
}
