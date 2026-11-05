#include <assert.h>
#include <rtthread.h>

#include "app_config.h"

int rt_mutex_init(struct rt_mutex *mutex, const char *name, int flag)
{
    (void)mutex;
    (void)name;
    (void)flag;
    return 0;
}

int rt_mutex_take(struct rt_mutex *mutex, int timeout)
{
    (void)mutex;
    (void)timeout;
    return 0;
}

int rt_mutex_release(struct rt_mutex *mutex)
{
    (void)mutex;
    return 0;
}

int main(void)
{
    g_config.chg_en = 0;
    g_config.ictrl = 0;
    g_config.r = 0;
    g_config.g = 0;

    UpdateConfigFromCommand("CHG_EN", 1);
    UpdateConfigFromCommand("ICTRL", 80);
    assert(g_config.chg_en == 0 && g_config.ictrl == 0);

    UpdateConfigFromCommand("R", 40);
    UpdateConfigFromCommand("G", 40);
    assert(g_config.r == 0 && g_config.g == 0);
    return 0;
}
