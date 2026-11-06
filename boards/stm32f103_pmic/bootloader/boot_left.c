#include <stdint.h>
#include <string.h>
#include "stm32f103xb.h"

#define APP_START 0x08001000U
#define APP_END   0x0800FC00U
#define PAGE_SIZE 1024U
#define MAX_SIZE  (APP_END - APP_START)
#define RX_SIZE   192U

static uint8_t first_page[PAGE_SIZE];
static char frame[RX_SIZE];
static uint32_t image_size;
static uint32_t image_crc;
static uint32_t image_offset;
static uint8_t updating;

void SystemInit(void)
{
    SCB->VTOR = 0x08000000U;
}

static void uart_putc(char c)
{
    while (!(USART3->SR & USART_SR_TXE)) {}
    USART3->DR = (uint16_t)c;
}

static void reply(const char *s)
{
    while (*s) uart_putc(*s++);
    while (!(USART3->SR & USART_SR_TC)) {}
}

static void reply_number(uint32_t number)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + number % 10U);
        number /= 10U;
    } while (number);
    while (count) uart_putc(digits[--count]);
    uart_putc(';');
}

static int parse_number(const char **p, uint32_t *value)
{
    uint32_t result = 0;
    unsigned count = 0;
    while (**p >= '0' && **p <= '9') {
        uint32_t digit = (uint32_t)(*(*p)++ - '0');
        if (result > (UINT32_MAX - digit) / 10U) return -1;
        result = result * 10U + digit;
        count++;
    }
    if (!count) return -1;
    *value = result;
    return 0;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static uint32_t crc_update(uint32_t crc, uint8_t byte)
{
    crc ^= byte;
    for (unsigned bit = 0; bit < 8; bit++)
        crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320U : 0U);
    return crc;
}

static int flash_wait(void)
{
    uint32_t timeout = 1000000U;
    while ((FLASH->SR & FLASH_SR_BSY) && timeout--) {}
    return (FLASH->SR & (FLASH_SR_BSY | FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) ? -1 : 0;
}

static void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123U;
        FLASH->KEYR = 0xCDEF89ABU;
    }
}

static int erase_page(uint32_t address)
{
    if (flash_wait()) return -1;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = address;
    FLASH->CR |= FLASH_CR_STRT;
    int result = flash_wait();
    FLASH->CR &= ~FLASH_CR_PER;
    if (result) return -1;
    for (unsigned i = 0; i < PAGE_SIZE; i += 2)
        if (*(volatile uint16_t *)(address + i) != 0xFFFFU) return -1;
    return 0;
}

static int write_halfword(uint32_t address, uint16_t word)
{
    if (flash_wait()) return -1;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
    FLASH->CR |= FLASH_CR_PG;
    *(volatile uint16_t *)address = word;
    int result = flash_wait();
    FLASH->CR &= ~FLASH_CR_PG;
    return result || *(volatile uint16_t *)address != word ? -1 : 0;
}

static int app_valid(void)
{
    uint32_t sp = *(volatile uint32_t *)APP_START;
    uint32_t pc = *(volatile uint32_t *)(APP_START + 4U);
    return sp >= 0x20000000U && sp <= 0x20005000U && (sp & 3U) == 0 &&
           pc >= APP_START && pc < APP_END && (pc & 1U);
}

static void jump_to_app(void)
{
    uint32_t sp = *(volatile uint32_t *)APP_START;
    uint32_t pc = *(volatile uint32_t *)(APP_START + 4U);
    SysTick->CTRL = 0;
    USART3->CR1 = 0;
    SCB->VTOR = APP_START;
    __set_MSP(sp);
    ((void (*)(void))pc)();
}

static void handle_frame(const char *s)
{
    if (strcmp(s, "OTA_PING:1") == 0) {
        reply("OTA_BOOT:1;");
        return;
    }
    if (strncmp(s, "OTA_BEGIN:", 10) == 0) {
        const char *p = s + 10;
        uint32_t size, crc;
        if (parse_number(&p, &size) || *p++ != ',' ||
            parse_number(&p, &crc) || *p || size < 8U || size > MAX_SIZE) {
            reply("OTA_ERR;");
            return;
        }
        GPIOB->BRR = GPIO_BRR_BR3;
        flash_unlock();
        updating = 0;
        if (erase_page(APP_START)) { reply("OTA_ERR;"); return; }
        image_size = size;
        image_crc = crc;
        image_offset = 0;
        updating = 1;
        reply("OTA_READY;");
        return;
    }
    if (strncmp(s, "OTA_DATA:", 9) == 0 && updating) {
        const char *p = s + 9;
        uint32_t offset;
        uint8_t bytes[64];
        unsigned count = 0;
        if (parse_number(&p, &offset) || *p++ != ',' || offset != image_offset) {
            reply("OTA_ERR;"); return;
        }
        while (*p && count < sizeof(bytes)) {
            int hi = hex_digit(*p++);
            if (!*p) { reply("OTA_ERR;"); return; }
            int lo = hex_digit(*p++);
            if (hi < 0 || lo < 0) { reply("OTA_ERR;"); return; }
            bytes[count++] = (uint8_t)((hi << 4) | lo);
        }
        if (*p || !count || offset % 64U || count > image_size - offset ||
            (count != 64U && offset + count != image_size) ||
            (count & 1U && offset + count != image_size)) {
            reply("OTA_ERR;"); return;
        }
        for (unsigned i = 0; i < count; i++) {
            uint32_t address = offset + i;
            if (address < PAGE_SIZE) first_page[address] = bytes[i];
        }
        for (unsigned i = 0; i < count; i += 2) {
            uint32_t address = offset + i;
            if (address < PAGE_SIZE) continue;
            if ((address % PAGE_SIZE) == 0 && erase_page(APP_START + address)) {
                updating = 0; reply("OTA_ERR;"); return;
            }
            uint16_t word = bytes[i] | (uint16_t)((i + 1U < count ? bytes[i + 1U] : 0xFFU) << 8);
            if (write_halfword(APP_START + address, word)) {
                updating = 0; reply("OTA_ERR;"); return;
            }
        }
        image_offset += count;
        reply("OTA_ACK:"); reply_number(image_offset);
        return;
    }
    if (strcmp(s, "OTA_END:1") == 0 && updating && image_offset == image_size) {
        uint32_t crc = 0xFFFFFFFFU;
        uint32_t sp, pc;
        for (uint32_t i = 0; i < image_size; i++)
            crc = crc_update(crc, i < PAGE_SIZE ? first_page[i] : *(volatile uint8_t *)(APP_START + i));
        memcpy(&sp, first_page, 4);
        memcpy(&pc, first_page + 4, 4);
        if ((crc ^ 0xFFFFFFFFU) != image_crc ||
            sp <= 0x20000000U || sp > 0x20005000U || (sp & 7U) ||
            pc < APP_START || pc >= APP_START + image_size || !(pc & 1U)) {
            updating = 0; reply("OTA_ERR;"); return;
        }
        for (uint32_t i = 8; i < PAGE_SIZE && i < image_size; i += 2) {
            uint16_t word = first_page[i] | (uint16_t)((i + 1 < image_size ? first_page[i + 1] : 0xFFU) << 8);
            if (write_halfword(APP_START + i, word)) { updating = 0; reply("OTA_ERR;"); return; }
        }
        if (write_halfword(APP_START + 4U, (uint16_t)pc) ||
            write_halfword(APP_START + 6U, (uint16_t)(pc >> 16)) ||
            write_halfword(APP_START, (uint16_t)sp) ||
            write_halfword(APP_START + 2U, (uint16_t)(sp >> 16))) {
            updating = 0; reply("OTA_ERR;"); return;
        }
        FLASH->CR |= FLASH_CR_LOCK;
        updating = 0;
        reply("OTA_DONE;");
        NVIC_SystemReset();
    }
    else if (strcmp(s, "OTA_ABORT:1") == 0) {
        updating = 0;
        FLASH->CR |= FLASH_CR_LOCK;
        reply("OTA_ABORTED;");
    }
    else reply("OTA_ERR;");
}

void boot_main(void)
{
    /* HSI remains at 8 MHz after reset. Keep the charger disabled. */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN;
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG) | AFIO_MAPR_SWJ_CFG_1;
    GPIOB->CRL = (GPIOB->CRL & ~(0xFU << 12)) | (0x2U << 12);
    GPIOB->BRR = GPIO_BRR_BR3;
    GPIOB->CRH = (GPIOB->CRH & ~((0xFU << 8) | (0xFU << 12))) |
                  (0xBU << 8) | (0x4U << 12);
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    USART3->BRR = 8000000U / 115200U;
    USART3->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    SysTick->LOAD = 7999U;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    reply("OTA_BOOT:1;");

    unsigned length = 0;
    uint32_t idle_ms = 0;
    while (1) {
        if (USART3->SR & USART_SR_RXNE) {
            char c = (char)USART3->DR;
            idle_ms = 0;
            if (c == ';') {
                frame[length] = 0;
                handle_frame(frame);
                length = 0;
            } else if (length < RX_SIZE - 1U) frame[length++] = c;
            else length = 0;
        }
        if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) {
            if (idle_ms < 3000U) idle_ms++;
            if (idle_ms >= 2000U && !updating && app_valid()) jump_to_app();
        }
    }
}
