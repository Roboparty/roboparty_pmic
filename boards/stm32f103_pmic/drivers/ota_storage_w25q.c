#include "ota_storage.h"
#include "w25q128.h"

int ota_storage_init(void)
{
    return w25q_init();
}

int ota_storage_erase(uint32_t offset, uint32_t length)
{
    uint32_t first = offset & ~(W25Q_SECTOR_SIZE - 1U);
    uint32_t last = (offset + length + W25Q_SECTOR_SIZE - 1U) & ~(W25Q_SECTOR_SIZE - 1U);

    for (uint32_t address = first; address < last; address += W25Q_SECTOR_SIZE)
    {
        w25q_erase_sector(address);
    }
    return 0;
}

int ota_storage_write(uint32_t offset, const uint8_t *data, uint32_t length)
{
    return w25q_write(offset, data, length);
}

int ota_storage_read(uint32_t offset, uint8_t *data, uint32_t length)
{
    return w25q_read(offset, data, length);
}

int ota_storage_invalidate(void)
{
    return w25q_erase_sector(0U);
}
