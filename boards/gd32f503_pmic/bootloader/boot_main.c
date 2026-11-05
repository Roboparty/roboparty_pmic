#include "gd32f50x.h"
#include "gd32f50x_fmc.h"
#include "ota.h"
#include "ota_partition.h"

#define GD32_FMC_PAGE_SIZE     4096U
#define BOOT_CHUNK_SIZE        1024U

static uint8_t chunk[BOOT_CHUNK_SIZE];

static uint32_t crc_region(uint32_t start, uint32_t length)
{
    const uint8_t *image = (const uint8_t *)start;
    uint32_t crc = OTA_CRC32_INIT;

    for (uint32_t offset = 0; offset < length; )
    {
        uint32_t count = length - offset;
        if (count > sizeof(chunk))
        {
            count = sizeof(chunk);
        }
        for (uint32_t i = 0; i < count; i++)
        {
            chunk[i] = image[offset + i];
        }
        crc = ota_crc32_update(crc, chunk, count);
        offset += count;
    }

    return ota_crc32_final(crc);
}

static int vector_valid(uint32_t stack, uint32_t entry)
{
    return stack >= 0x20000000U && stack <= 0x20010000U &&
           (stack & 7U) == 0U &&
           (entry & 1U) != 0U &&
           (entry & ~1U) >= GD32_APP_START &&
           (entry & ~1U) < GD32_APP_START + GD32_APP_SIZE;
}

static void jump_to_app(void)
{
    uint32_t stack = *(volatile uint32_t *)GD32_APP_START;
    uint32_t entry = *(volatile uint32_t *)(GD32_APP_START + 4U);

    if (!vector_valid(stack, entry))
    {
        while (1)
        {
        }
    }

    SCB->VTOR = GD32_APP_START;
    __set_MSP(stack);
    __enable_irq();
    ((void (*)(void))entry)();
}

void boot_main(void)
{
    const ota_header_t *header = (const ota_header_t *)GD32_STAGING_START;
    const uint8_t *image = (const uint8_t *)(GD32_STAGING_START + OTA_PAYLOAD_OFFSET);

    SCB->VTOR = GD32_BOOT_START;

    if (ota_header_check(header) != 0 || header->entry != GD32_APP_START ||
        header->length < 8U ||
        header->length > GD32_APP_SIZE ||
        header->length > GD32_STAGING_SIZE - OTA_PAYLOAD_OFFSET)
    {
        jump_to_app();
    }

    if (crc_region(GD32_STAGING_START + OTA_PAYLOAD_OFFSET,
                   header->length) != header->crc32 ||
        !vector_valid(*(const uint32_t *)(image),
                      *(const uint32_t *)(image + 4U)))
    {
        fmc_unlock();
        (void)fmc_page_erase(GD32_STAGING_START);
        fmc_lock();
        jump_to_app();
    }

    fmc_unlock();
    for (uint32_t address = GD32_APP_START;
         address < (GD32_APP_START + GD32_APP_SIZE);
         address += GD32_FMC_PAGE_SIZE)
    {
        if (fmc_page_erase(address) != FMC_READY)
        {
            fmc_lock();
            while (1) { }
        }
    }

    for (uint32_t offset = 0; offset < header->length; offset += 2U)
    {
        uint16_t word = image[offset];
        if ((offset + 1U) < header->length)
        {
            word |= (uint16_t)(image[offset + 1U] << 8);
        }
        else
        {
            word |= 0xFF00U;
        }
        if (fmc_halfword_program(GD32_APP_START + offset, word) != FMC_READY)
        {
            fmc_lock();
            while (1) { }
        }
    }
    fmc_lock();

    if (crc_region(GD32_APP_START, header->length) != header->crc32)
    {
        while (1) { }
    }

    fmc_unlock();
    (void)fmc_page_erase(GD32_STAGING_START);
    fmc_lock();

    jump_to_app();
}
