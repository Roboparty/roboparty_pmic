#include "rtthread.h"

int main(void)
{
    rt_kprintf("roboparty_pmic GD32F503\n");
    while (1) {
        rt_thread_mdelay(1000);
    }
}
