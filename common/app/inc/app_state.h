#ifndef __APP_STATE_H__
#define __APP_STATE_H__

#include "app_types.h"
#include <rtthread.h>

extern env_secure_t *pEnvSecure;
extern env_secure_t g_env_secure;

extern float CHRG_I;
extern float CHRG_V;
extern float BAT_V;
extern float temp;

extern uint16_t CHRG_PWR;

extern uint16_t BmsSoc[1];
extern uint16_t BmsSoh[1];

extern uint8_t BAT_INST;
extern uint8_t IR_REC;
extern uint8_t BMS_COM_ERR;
extern uint8_t CHRG_FULL;
extern uint8_t CLEAN_EXCP;

extern rt_mutex_t g_state_lock;

void app_state_init(void);
void app_state_lock(void);
void app_state_unlock(void);

#endif /* __APP_STATE_H__ */
