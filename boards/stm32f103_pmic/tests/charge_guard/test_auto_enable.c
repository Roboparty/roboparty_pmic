#include <assert.h>

#include "app_charge_policy.h"

int main(void)
{
    assert(app_left_charge_ready(20.0f, 48.0f, 1.0f, 25.0f, 3.0f, 3, 0));
    assert(!app_left_charge_ready(20.0f, 48.0f, 1.0f, 25.0f, 3.0f, 2, 0));
    assert(!app_left_charge_ready(15.9f, 48.0f, 1.0f, 25.0f, 3.0f, 3, 0));
    assert(!app_left_charge_ready(32.0f, 48.0f, 1.0f, 25.0f, 3.0f, 3, 0));
    assert(!app_left_charge_ready(20.0f, 55.0f, 1.0f, 25.0f, 3.0f, 3, 0));
    assert(!app_left_charge_ready(20.0f, 48.0f, 6.0f, 25.0f, 3.0f, 3, 0));
    assert(!app_left_charge_ready(20.0f, 48.0f, 1.0f, 80.0f, 3.0f, 3, 0));
    assert(!app_left_charge_ready(20.0f, 48.0f, 1.0f, 25.0f, 0.0f, 3, 0));
    assert(!app_left_charge_ready(20.0f, 48.0f, 1.0f, 25.0f, 3.0f, 3, 1));
    return 0;
}
