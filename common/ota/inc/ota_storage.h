#ifndef __OTA_STORAGE_H__
#define __OTA_STORAGE_H__

#include <stdint.h>

int ota_storage_init(void);
int ota_storage_erase(uint32_t offset, uint32_t length);
int ota_storage_write(uint32_t offset, const uint8_t *data, uint32_t length);
int ota_storage_read(uint32_t offset, uint8_t *data, uint32_t length);
int ota_storage_invalidate(void);

#endif /* __OTA_STORAGE_H__ */
