#ifndef __FLASH_DRIVER_H
#define __FLASH_DRIVER_H
#define FLASH_START_ADDR 0x08000000U
#define FLASH_END_ADDR 0X080E0000U
#include "flash_partition.h"

#include "stm32f4xx_hal.h"
typedef enum{
    FLASH_OK=0,
    FLASH_ERR_ADDR=1,
    FLASH_ERR_ERASE=2,
    FLASH_ERR_WRITE=3,
    FLASH_ERR_VERIFY=4,
    FLASH_ERR_NOT_BLANK=5
}Flash_Status_Type;

uint32_t Flash_GetSector(uint32_t addr);

Flash_Status_Type Flash_EraseSector(uint32_t sector_addr);

Flash_Status_Type Flash_Write(uint32_t dst_addr,uint8_t *data_buf,uint32_t len_byte,uint32_t *next_wr_addr);

Flash_Status_Type Flash_CheckBlank(uint32_t addr,uint32_t len_byte);

#endif