#include "bsp.h"
#include "gd32f50x_fmc.h"

int bsp_flash_erase_page(uint32_t address)
{
    fmc_unlock();
    fmc_state_enum result = fmc_page_erase(address);
    fmc_lock();
    return result == FMC_READY ? 0 : -1;
}

int bsp_flash_write_halfword(uint32_t address, uint16_t data)
{
    fmc_unlock();
    fmc_state_enum result = fmc_halfword_program(address, data);
    fmc_lock();
    return result == FMC_READY ? 0 : -1;
}
