#ifndef __BOOT_JUMP_H
#define __BOOT_JUMP_H

#include "stm32f4xx_hal.h"
#include "flash_partition.h"
#include "flash_driver.h"

typedef enum
{
    JUMP_OK=0,
    JUMP_ERR_SP_INVALID=1,
    JUMP_ERR_PC_THUMB=2,
    JUMP_ERR_PC_OUT_RANGE=3
}BOOT_JUMP_RET;

BOOT_JUMP_RET Boot_jump_Exec(uint32_t slot_base);

typedef struct 
{
    uint32_t magic;
    uint32_t run_slot;
    uint32_t update_flag;

}OtaMetaTypeDef __attribute__((aligned(4)));

void Meta_Read(OtaMetaTypeDef *meta);
void Meta_Write(OtaMetaTypeDef *meta);

BOOT_JUMP_RET Check_App_Valid(uint32_t app_addr);

#endif