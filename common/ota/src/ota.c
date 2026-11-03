#include "ota.h"

uint32_t ota_crc32_update(uint32_t crc, const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++)
        {
            if (crc & 1U)
            {
                crc = (crc >> 1) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint32_t ota_crc32_final(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFU;
}

uint32_t ota_crc32(const uint8_t *data, uint32_t length)
{
    return ota_crc32_final(ota_crc32_update(OTA_CRC32_INIT, data, length));
}

void ota_header_fill(ota_header_t *header, uint32_t length, uint32_t crc,
                     uint32_t entry, uint32_t version)
{
    header->magic = OTA_MAGIC;
    header->version = version;
    header->length = length;
    header->crc32 = crc;
    header->entry = entry;
    for (uint8_t i = 0; i < sizeof(header->reserved); i++)
    {
        header->reserved[i] = 0xFFU;
    }
}

int ota_header_check(const ota_header_t *header)
{
    return (header->magic == OTA_MAGIC) && (header->length != 0U) &&
           (header->entry != 0U) ? 0 : -1;
}
