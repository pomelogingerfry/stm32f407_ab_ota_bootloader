#ifndef _FLASH_DRIVER_H
#define _FLASH_DRIVER_H
#define FLASH_START_ADDR 0x08000000U
#define FLASH_END_ADDR 0x08100000U

#include "flash_partition.h"
#include "stm32f4xx_hal.h"

typedef enum{
    FLASH_OK=0,
    FLASH_ERR_ADDR=1,
    FLASH_ERR_ERASE=2,
    FLASH_ERR_WRITE=3,
    FLASH_ERR_VERIFY=4,
    FLASH_ERR_NOT_BLANK=5
}FLASH_STATUS_TYPE;

uint32_t Flash_GetSector(uint32_t addr);

FLASH_STATUS_TYPE Flash_EraseSector(uint32_t sector_addr);

FLASH_STATUS_TYPE Flash_Write(uint32_t dest_addr,uint8_t *data_buf,
    uint32_t len_byte,uint32_t *next_wr_addr);

FLASH_STATUS_TYPE Flash_CheckBlank(uint32_t addr,uint32_t len_byte);

#endif