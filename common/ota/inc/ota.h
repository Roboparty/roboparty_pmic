#ifndef __OTA_H__
#define __OTA_H__

#include <stdint.h>

#define OTA_MAGIC             0x4F544131U   /* "OTA1" */
#define OTA_HEADER_SIZE       32U
#define OTA_PAYLOAD_OFFSET    0x100U
#define OTA_CRC32_INIT        0xFFFFFFFFU

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t length;
    uint32_t crc32;
    uint32_t entry;
    uint8_t  reserved[12];
} ota_header_t;

uint32_t ota_crc32(const uint8_t *data, uint32_t length);
uint32_t ota_crc32_update(uint32_t crc, const uint8_t *data, uint32_t length);
uint32_t ota_crc32_final(uint32_t crc);
void ota_header_fill(ota_header_t *header, uint32_t length, uint32_t crc,
                     uint32_t entry, uint32_t version);
int ota_header_check(const ota_header_t *header);

#endif /* __OTA_H__ */
