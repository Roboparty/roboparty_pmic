#ifndef __OTA_PARTITION_H__
#define __OTA_PARTITION_H__

#define GD32_BOOT_START        0x08000000U
#define GD32_BOOT_SIZE         (32U * 1024U)

#define GD32_APP_START         0x08008000U
#define GD32_APP_SIZE          (224U * 1024U)

#define GD32_STAGING_START     0x08040000U
#define GD32_STAGING_SIZE      (224U * 1024U)

#define GD32_CONFIG_START      0x08078000U
#define GD32_CONFIG_SIZE       (32U * 1024U)

#endif /* __OTA_PARTITION_H__ */
