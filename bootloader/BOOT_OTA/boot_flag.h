#ifndef __BOOT_FLAG_H
#define __BOOT_FLAG_H

#include <stdint.h>

#define OTA_FLAG_MAGIC 0x44554554

#define FLAG_STATE_IDLE 0
#define FLAG_STATE_TRIAL 1
#define FLAG_STATE_CONFIRMED 2

typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint32_t seq;
    uint8_t active_slot;
    uint8_t pending_slot;
    uint16_t try_count;
    uint32_t fw_size;
    uint32_t fw_crc32;
    uint32_t state;
    uint32_t flag_crc;
    uint32_t reserved[5];
} boot_flag_ts;
 
static uint32_t bootflag_CRC32_calc(const boot_flag_ts *flag);

void BootFlag_Read(boot_flag_ts *flag);

void BootFlag_Write(boot_flag_ts *flag);

#endif