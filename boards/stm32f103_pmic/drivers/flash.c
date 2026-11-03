#include "board.h"
#include "bsp.h"

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

int bsp_flash_erase_page(uint32_t address)
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

int bsp_flash_write_halfword(uint32_t address, uint16_t data)
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
