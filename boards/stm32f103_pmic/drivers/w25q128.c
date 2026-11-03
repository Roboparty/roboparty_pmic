#include "stm32f103xb.h"
#include "w25q128.h"

#define W25Q_CMD_WREN       0x06U
#define W25Q_CMD_RDSR1      0x05U
#define W25Q_CMD_READ       0x03U
#define W25Q_CMD_PP         0x02U
#define W25Q_CMD_SE         0x20U
#define W25Q_CMD_RDID       0x9FU

#define W25Q_CS_LOW()       (GPIOB->BRR = GPIO_BRR_BR12)
#define W25Q_CS_HIGH()      (GPIOB->BSRR = GPIO_BSRR_BS12)

static void spi2_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* PB13 SCK, PB15 MOSI: AF push-pull 50 MHz; PB14 MISO input; PB12 CS output. */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFFU << 16)) |
                 (0xBU << 20) | (0x4U << 24) | (0xBU << 28);
    GPIOB->CRH = (GPIOB->CRH & ~(0xFU << 16)) | (0x2U << 16);
    W25Q_CS_HIGH();

    SPI2->CR1 = 0U;
    SPI2->CR2 = 0U;
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BR_0 | SPI_CR1_SPE;
}

static uint8_t spi2_xfer(uint8_t data)
{
    while ((SPI2->SR & SPI_SR_TXE) == 0U)
    {
    }
    *(volatile uint8_t *)&SPI2->DR = data;
    while ((SPI2->SR & SPI_SR_RXNE) == 0U)
    {
    }
    return *(volatile uint8_t *)&SPI2->DR;
}

static uint8_t w25q_read_status(void)
{
    uint8_t status;

    W25Q_CS_LOW();
    (void)spi2_xfer(W25Q_CMD_RDSR1);
    status = spi2_xfer(0xFFU);
    W25Q_CS_HIGH();
    return status;
}

static void w25q_wait_busy(void)
{
    while ((w25q_read_status() & 0x01U) != 0U)
    {
    }
}

static void w25q_write_enable(void)
{
    W25Q_CS_LOW();
    (void)spi2_xfer(W25Q_CMD_WREN);
    W25Q_CS_HIGH();
}

int w25q_init(void)
{
    spi2_init();
    for (uint32_t attempt = 0; attempt < 100000U; attempt++)
    {
        if ((w25q_read_status() & 0x01U) == 0U)
        {
            return 0;
        }
    }
    return -1;
}

void w25q_read_id(uint8_t id[3])
{
    W25Q_CS_LOW();
    (void)spi2_xfer(W25Q_CMD_RDID);
    id[0] = spi2_xfer(0xFFU);
    id[1] = spi2_xfer(0xFFU);
    id[2] = spi2_xfer(0xFFU);
    W25Q_CS_HIGH();
}

int w25q_read(uint32_t address, uint8_t *data, uint32_t length)
{
    W25Q_CS_LOW();
    (void)spi2_xfer(W25Q_CMD_READ);
    (void)spi2_xfer((uint8_t)(address >> 16));
    (void)spi2_xfer((uint8_t)(address >> 8));
    (void)spi2_xfer((uint8_t)address);
    for (uint32_t i = 0; i < length; i++)
    {
        data[i] = spi2_xfer(0xFFU);
    }
    W25Q_CS_HIGH();
    return 0;
}

static int w25q_page_program(uint32_t address, const uint8_t *data, uint16_t length)
{
    w25q_write_enable();

    W25Q_CS_LOW();
    (void)spi2_xfer(W25Q_CMD_PP);
    (void)spi2_xfer((uint8_t)(address >> 16));
    (void)spi2_xfer((uint8_t)(address >> 8));
    (void)spi2_xfer((uint8_t)address);
    for (uint16_t i = 0; i < length; i++)
    {
        (void)spi2_xfer(data[i]);
    }
    W25Q_CS_HIGH();

    w25q_wait_busy();
    return 0;
}

int w25q_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    while (length > 0U)
    {
        uint32_t page_left = W25Q_PAGE_SIZE - (address % W25Q_PAGE_SIZE);
        uint16_t chunk = (uint16_t)((length < page_left) ? length : page_left);

        w25q_page_program(address, data, chunk);
        address += chunk;
        data += chunk;
        length -= chunk;
    }
    return 0;
}

int w25q_erase_sector(uint32_t address)
{
    w25q_write_enable();

    W25Q_CS_LOW();
    (void)spi2_xfer(W25Q_CMD_SE);
    (void)spi2_xfer((uint8_t)(address >> 16));
    (void)spi2_xfer((uint8_t)(address >> 8));
    (void)spi2_xfer((uint8_t)address);
    W25Q_CS_HIGH();

    w25q_wait_busy();
    return 0;
}
