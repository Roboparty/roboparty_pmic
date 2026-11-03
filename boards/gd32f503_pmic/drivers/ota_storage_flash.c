#include <string.h>

#include "ota_storage.h"
#include "ota_partition.h"
#include "gd32f50x.h"
#include "gd32f50x_fmc.h"

#define GD32_FMC_PAGE_SIZE     4096U

int ota_storage_init(void)
{
    return 0;
}

int ota_storage_erase(uint32_t offset, uint32_t length)
{
    uint32_t first = offset & ~(GD32_FMC_PAGE_SIZE - 1U);
    uint32_t last = (offset + length + GD32_FMC_PAGE_SIZE - 1U) &
                    ~(GD32_FMC_PAGE_SIZE - 1U);

    fmc_unlock();
    for (uint32_t address = first; address < last; address += GD32_FMC_PAGE_SIZE)
    {
        (void)fmc_page_erase(GD32_STAGING_START + address);
    }
    fmc_lock();
    return 0;
}

int ota_storage_write(uint32_t offset, const uint8_t *data, uint32_t length)
{
    fmc_unlock();
    for (uint32_t i = 0; i < length; i += 2U)
    {
        uint16_t word = data[i];
        if ((i + 1U) < length)
        {
            word |= (uint16_t)(data[i + 1U] << 8);
        }
        else
        {
            word |= 0xFF00U;
        }
        (void)fmc_halfword_program(GD32_STAGING_START + offset + i, word);
    }
    fmc_lock();
    return 0;
}

int ota_storage_read(uint32_t offset, uint8_t *data, uint32_t length)
{
    memcpy(data, (const void *)(GD32_STAGING_START + offset), length);
    return 0;
}

int ota_storage_invalidate(void)
{
    fmc_unlock();
    (void)fmc_page_erase(GD32_STAGING_START);
    fmc_lock();
    return 0;
}
