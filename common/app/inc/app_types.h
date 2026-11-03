#ifndef __APP_TYPES_H__
#define __APP_TYPES_H__

#include <stdio.h>

#include "bsp.h"
#include "pinout.h"

typedef struct
{
    float estimate_temp;
    float estimate_chrg_volt;
    float estimate_chrg_current;
    float estimate_bat_volt;
    float estimate_pre_chrg_volt;
    uint8_t exception;
} env_secure_t;

extern env_secure_t *pEnvSecure;
extern float CHRG_I, CHRG_V, BAT_V;
extern uint8_t BAT_INST, IR_REC;

#endif /* __APP_TYPES_H__ */
