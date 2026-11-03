#ifndef __W25Q128_H__
#define __W25Q128_H__

#include <stdint.h>

#define W25Q_SECTOR_SIZE    4096U
#define W25Q_PAGE_SIZE      256U

int w25q_init(void);
void w25q_read_id(uint8_t id[3]);
int w25q_read(uint32_t address, uint8_t *data, uint32_t length);
int w25q_write(uint32_t address, const uint8_t *data, uint32_t length);
int w25q_erase_sector(uint32_t address);

#endif /* __W25Q128_H__ */
