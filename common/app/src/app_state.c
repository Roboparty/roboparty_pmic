#include "app_state.h"

env_secure_t g_env_secure;
env_secure_t *pEnvSecure;

float CHRG_I = 0.0f;
float CHRG_V = 0.0f;
float BAT_V = 0.0f;
float temp = 0.0f;

uint16_t CHRG_PWR = 0;

uint16_t BmsSoc[1] = {0};
uint16_t BmsSoh[1] = {0};

uint8_t BAT_INST = 0;
uint8_t IR_REC = 0;
uint8_t BMS_COM_ERR = 0;
uint8_t CHRG_FULL = 0;
uint8_t CLEAN_EXCP = 0;

static struct rt_mutex state_mutex;
rt_mutex_t g_state_lock;

void app_state_init(void)
{
    pEnvSecure = &g_env_secure;
    pEnvSecure->exception = 0;

    rt_mutex_init(&state_mutex, "state", RT_IPC_FLAG_PRIO);
    g_state_lock = &state_mutex;
}

void app_state_lock(void)
{
    rt_mutex_take(g_state_lock, RT_WAITING_FOREVER);
}

void app_state_unlock(void)
{
    rt_mutex_release(g_state_lock);
}
