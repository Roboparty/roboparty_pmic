#ifndef __BMS_H__
#define __BMS_H__

#include <stdint.h>

int bms_init(void);
int bms_read_soc(uint16_t *soc);
int bms_read_soh(uint16_t *soh);

#endif /* __BMS_H__ */
