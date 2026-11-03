#include <rtthread.h>

#include "app_bms.h"
#include "app_state.h"

#include "bms.h"

void app_bms_thread_entry(void *parameter)
{
    (void)parameter;

    bms_init();

    while (1)
    {
        uint8_t bat_inst;
        float chrg_v;

        app_state_lock();
        bat_inst = BAT_INST;
        chrg_v = CHRG_V;
        app_state_unlock();

        if (bat_inst && chrg_v >= 4.0f)
        {
            uint16_t soc = 0;
            uint16_t soh = 0;

            if (bms_read_soc(&soc))
            {
                app_state_lock();
                BMS_COM_ERR = 1;
                app_state_unlock();
            }
            else
            {
                app_state_lock();
                BmsSoc[0] = soc;
                app_state_unlock();
            }

            rt_thread_mdelay(100);

            if (bms_read_soh(&soh))
            {
                app_state_lock();
                BMS_COM_ERR = 1;
                app_state_unlock();
            }
            else
            {
                app_state_lock();
                BmsSoh[0] = soh;
                app_state_unlock();
            }
        }
        else
        {
            app_state_lock();
            BmsSoc[0] = 0;
            BmsSoh[0] = 0;
            app_state_unlock();
        }

        rt_thread_mdelay(1000);
    }
}
