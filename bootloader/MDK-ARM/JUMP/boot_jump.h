#ifndef _BOOT_JUMP_H
#define _BOOT_JUMP_H

#include "stm32f4xx_hal.h"

#include "flash_partition.h"

typedef enum
{
    JUMP_OK=0,
    JUMP_ERR_SP_INVALID=1,
    JUMP_ERR_PC_THUMB=2,
    JUMP_ERR_PC_OUT_RANGE=3
}BOOT_JUMP_RET;

BOOT_JUMP_RET Boot_jump_Exec(uint32_t slot_base);

#endif