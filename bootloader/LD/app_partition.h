#ifndef __APP_PARTITION_H
#define __APP_PARTITION_H
#include "stdint.h"

#define BOOT_BASE        0x08000000UL
#define BOOT_SIZE        (128UL * 1024UL)

#define META_BASE        0x08020000UL
#define META_SIZE        (128UL * 1024UL)

#define APP_A_BASE       0x08040000UL
#define APP_A_SIZE       (256UL * 1024UL)

#define APP_B_BASE       0x08080000UL
#define APP_B_SIZE       (256UL * 1024UL)

#define FAULT_BASE       0x080C0000UL
#define FAULT_SIZE       (128UL * 1024UL)

#define META_MIRROR_BASE 0x080E0000UL
#define META_MIRROR_SIZE (128UL * 1024UL)

typedef enum
{
    BOOT_FLAG_RUN_A = 0,
    BOOT_FLAG_COPY_B_TO_A = 1,
} BootFlag_t;

typedef struct
{
    uint32_t flag;
    uint32_t app_crc32;
    uint32_t app_len;
    uint32_t magic;
} MetaInfo_t;

#define META_MAGIC 0xA5A5A5A5UL

#endif
