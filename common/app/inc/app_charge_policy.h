#ifndef __APP_CHARGE_POLICY_H__
#define __APP_CHARGE_POLICY_H__

#include <stdint.h>

static inline uint8_t app_left_charge_ready(float input_v, float battery_v,
                                            float charge_i, float temperature,
                                            float adapter_current, uint8_t valid_samples,
                                            uint8_t exception)
{
    return valid_samples >= 3U && exception == 0U &&
           input_v >= 16.0f && input_v < 32.0f &&
           battery_v < 55.0f && charge_i < 6.0f && temperature < 80.0f &&
           adapter_current > 0.0f;
}

static inline uint8_t app_left_charge_led(float battery_v, uint8_t charge_enabled)
{
    if (!charge_enabled)
    {
        return 0U;
    }
    return battery_v >= 54.0f ? 2U : 1U; /* 1: PA4 green, 2: PA5 red */
}

static inline uint8_t app_left_battery_led(float pa1_v, uint8_t sample_valid)
{
    if (!sample_valid)
    {
        return 0U;
    }
    if (pa1_v >= 1.39f && pa1_v <= 1.50f)
    {
        return 1U; /* PA6 */
    }
    if (pa1_v >= 1.21f && pa1_v < 1.39f)
    {
        return 2U; /* PA7 */
    }
    return 0U;
}

#endif /* __APP_CHARGE_POLICY_H__ */
