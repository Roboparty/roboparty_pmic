#include <rtthread.h>
#include <rtdevice.h>

#include "bsp.h"
#include "bms.h"

#define SCUD_HEAD           0xFEU
#define SCUD_TAIL           0xBBU
#define SCUD_CMD_USER       0x61U
#define SCUD_LEN_USER       30U
#define SCUD_CMD_SUMMARY    0x31U
#define SCUD_LEN_SUMMARY    39U
#define SCUD_BAUDRATE       19200U
#define SCUD_TIMEOUT_MS     500U

static rt_device_t scud_dev;
static uint8_t scud_payload[SCUD_LEN_SUMMARY];

static uint16_t crc16_ccitt(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0U;

    for (uint16_t i = 0; i < length; i++)
    {
        crc = (uint16_t)((crc >> 8) | (crc << 8));
        crc ^= data[i];
        crc ^= (uint16_t)((crc & 0xFFU) >> 4);
        crc ^= (uint16_t)((crc << 8) << 4);
        crc ^= (uint16_t)(((crc & 0xFFU) << 4) << 1);
    }

    return crc;
}

static int scud_send(uint8_t cmd)
{
    uint8_t body[3] = {cmd, 1U, 1U};
    uint16_t crc = crc16_ccitt(body, sizeof(body));
    uint8_t frame[8] = {
        SCUD_HEAD, SCUD_HEAD, cmd, 1U, 1U,
        (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFFU), SCUD_TAIL,
    };

    if (scud_dev == RT_NULL)
    {
        return -1;
    }

    return rt_device_write(scud_dev, 0, frame, sizeof(frame)) == sizeof(frame) ? 0 : -1;
}

static int scud_read_frame(uint8_t expect_cmd, uint8_t expect_len,
                           uint8_t *payload, uint32_t timeout_ms)
{
    enum
    {
        SCUD_WAIT_HEAD1,
        SCUD_WAIT_HEAD2,
        SCUD_WAIT_CMD,
        SCUD_WAIT_LEN,
        SCUD_WAIT_PAYLOAD,
        SCUD_WAIT_CRC_H,
        SCUD_WAIT_CRC_L,
        SCUD_WAIT_TAIL,
    } state = SCUD_WAIT_HEAD1;
    uint8_t cmd = 0U;
    uint8_t length = 0U;
    uint8_t crc_h = 0U;
    uint8_t crc_l = 0U;
    uint16_t offset = 0U;
    uint32_t start = bsp_tick_get();

    while ((bsp_tick_get() - start) < timeout_ms)
    {
        uint8_t byte;

        if (rt_device_read(scud_dev, 0, &byte, 1) != 1)
        {
            rt_thread_mdelay(1);
            continue;
        }

        switch (state)
        {
        case SCUD_WAIT_HEAD1:
            if (byte == SCUD_HEAD)
            {
                state = SCUD_WAIT_HEAD2;
            }
            break;

        case SCUD_WAIT_HEAD2:
            if (byte == SCUD_HEAD)
            {
                state = SCUD_WAIT_CMD;
            }
            else
            {
                state = SCUD_WAIT_HEAD1;
            }
            break;

        case SCUD_WAIT_CMD:
            if (byte == expect_cmd)
            {
                cmd = byte;
                state = SCUD_WAIT_LEN;
            }
            else if (byte == SCUD_HEAD)
            {
                state = SCUD_WAIT_CMD;
            }
            else
            {
                state = SCUD_WAIT_HEAD1;
            }
            break;

        case SCUD_WAIT_LEN:
            length = byte;
            if (length != expect_len)
            {
                state = (byte == SCUD_HEAD) ? SCUD_WAIT_HEAD2 : SCUD_WAIT_HEAD1;
                break;
            }
            offset = 0U;
            state = (length == 0U) ? SCUD_WAIT_CRC_H : SCUD_WAIT_PAYLOAD;
            break;

        case SCUD_WAIT_PAYLOAD:
            if (offset < expect_len)
            {
                payload[offset++] = byte;
            }
            if (offset >= length)
            {
                state = SCUD_WAIT_CRC_H;
            }
            break;

        case SCUD_WAIT_CRC_H:
            crc_h = byte;
            state = SCUD_WAIT_CRC_L;
            break;

        case SCUD_WAIT_CRC_L:
            crc_l = byte;
            state = SCUD_WAIT_TAIL;
            break;

        case SCUD_WAIT_TAIL:
            if ((cmd == expect_cmd) && (length == expect_len) && (byte == SCUD_TAIL))
            {
                uint8_t crc_buf[SCUD_LEN_SUMMARY + 2U];
                uint16_t crc;

                crc_buf[0] = cmd;
                crc_buf[1] = length;
                for (uint16_t i = 0; i < length; i++)
                {
                    crc_buf[i + 2U] = payload[i];
                }

                crc = crc16_ccitt(crc_buf, (uint16_t)(length + 2U));
                if (crc == (uint16_t)((crc_h << 8) | crc_l))
                {
                    return 0;
                }
            }
            state = (byte == SCUD_HEAD) ? SCUD_WAIT_HEAD2 : SCUD_WAIT_HEAD1;
            break;

        default:
            state = SCUD_WAIT_HEAD1;
            break;
        }
    }

    return -1;
}

static int scud_query(uint8_t cmd, uint8_t length, uint8_t *payload)
{
    if (scud_send(cmd) != 0)
    {
        return -1;
    }
    return scud_read_frame(cmd, length, payload, SCUD_TIMEOUT_MS);
}

int bms_init(void)
{
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

    scud_dev = rt_device_find("uart1");
    if (scud_dev == RT_NULL)
    {
        return -1;
    }

    cfg.baud_rate = BAUD_RATE_19200;
    cfg.data_bits = DATA_BITS_8;
    cfg.stop_bits = STOP_BITS_1;
    cfg.parity = PARITY_NONE;

    rt_device_control(scud_dev, RT_DEVICE_CTRL_CONFIG, &cfg);
    rt_device_open(scud_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    return 0;
}

int bms_read_soc(uint16_t *soc)
{
    if (scud_query(SCUD_CMD_USER, SCUD_LEN_USER, scud_payload) != 0)
    {
        return -1;
    }

    /* 0x61 payload offset 24: SOC percentage, big-endian u16. */
    *soc = (uint16_t)((scud_payload[24] << 8) | scud_payload[25]);
    return 0;
}

int bms_read_soh(uint16_t *soh)
{
    /* The SCUD 0x61/0x31 protocol does not carry SOH. */
    *soh = 0U;
    return 0;
}
