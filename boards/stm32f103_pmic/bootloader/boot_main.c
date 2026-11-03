#include "stm32f103xb.h"
#ifndef PMIC_LEFT_LAYOUT
#include "ota.h"
#include "ota_storage.h"
#include "w25q128.h"
#endif

#define APP_START_ADDR      0x08001000U
#ifndef PMIC_LEFT_LAYOUT
#define APP_END_ADDR        0x0800FC00U
#define FLASH_PAGE_SIZE     1024U
#define BOOT_CHUNK_SIZE     1024U

static uint8_t chunk[BOOT_CHUNK_SIZE];
#endif

void SystemInit(void)
{
    SCB->VTOR = 0x08000000U;
}

#ifndef PMIC_LEFT_LAYOUT
static void flash_wait(void)
{
    while ((FLASH->SR & FLASH_SR_BSY) != 0U)
    {
    }
}

static void flash_clear_flags(void)
{
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
}

static int flash_erase_page(uint32_t address)
{
    FLASH->KEYR = 0x45670123U;
    FLASH->KEYR = 0xCDEF89ABU;

    flash_wait();
    flash_clear_flags();
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = address;
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait();
    FLASH->CR &= ~FLASH_CR_PER;
    FLASH->CR |= FLASH_CR_LOCK;

    return (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) ? -1 : 0;
}

static int flash_program_halfword(uint32_t address, uint16_t data)
{
    FLASH->KEYR = 0x45670123U;
    FLASH->KEYR = 0xCDEF89ABU;

    flash_wait();
    flash_clear_flags();
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)address = data;
    flash_wait();
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->CR |= FLASH_CR_LOCK;

    return (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) ? -1 : 0;
}

static uint32_t crc_storage(uint32_t length)
{
    uint32_t crc = OTA_CRC32_INIT;

    for (uint32_t offset = 0; offset < length; )
    {
        uint32_t count = length - offset;
        if (count > sizeof(chunk))
        {
            count = sizeof(chunk);
        }
        w25q_read(OTA_PAYLOAD_OFFSET + offset, chunk, count);
        crc = ota_crc32_update(crc, chunk, count);
        offset += count;
    }

    return ota_crc32_final(crc);
}
#endif

static void jump_to_app(void)
{
    uint32_t stack = *(volatile uint32_t *)APP_START_ADDR;
    uint32_t entry = *(volatile uint32_t *)(APP_START_ADDR + 4U);

    if ((stack & 0xFFFE0000U) != 0x20000000U)
    {
        while (1)
        {
        }
    }

    SCB->VTOR = APP_START_ADDR;
    __set_MSP(stack);
    __enable_irq();
    ((void (*)(void))entry)();
}

void boot_main(void)
{
#ifdef PMIC_LEFT_LAYOUT
    jump_to_app();
#else
    ota_header_t header;
    uint32_t crc;

    if (w25q_init() != 0)
    {
        jump_to_app();
    }
    w25q_read(0U, (uint8_t *)&header, sizeof(header));

    if (ota_header_check(&header) != 0)
    {
        jump_to_app();
    }

    crc = crc_storage(header.length);
    if (crc != header.crc32)
    {
        ota_storage_invalidate();
        jump_to_app();
    }

    for (uint32_t address = APP_START_ADDR; address < APP_END_ADDR; address += FLASH_PAGE_SIZE)
    {
        flash_erase_page(address);
    }

    for (uint32_t offset = 0; offset < header.length; )
    {
        uint32_t count = header.length - offset;
        if (count > sizeof(chunk))
        {
            count = sizeof(chunk);
        }

        w25q_read(OTA_PAYLOAD_OFFSET + offset, chunk, count);

        for (uint32_t i = 0; i < count; i += 2U)
        {
            uint16_t word = chunk[i];
            if ((i + 1U) < count)
            {
                word |= (uint16_t)(chunk[i + 1U] << 8);
            }
            else
            {
                word |= 0xFF00U;
            }
            flash_program_halfword(APP_START_ADDR + offset + i, word);
        }

        offset += count;
    }

    ota_storage_invalidate();
    jump_to_app();
#endif
}
