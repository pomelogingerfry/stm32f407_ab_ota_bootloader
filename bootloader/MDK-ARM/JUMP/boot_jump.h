#ifndef __BOOT_JUMP_A
#define __BOOT_JUMP_A

#include "stdio.h"

#include "flash_partition.h"

uint8_t App_IsValid(uint32_t app_addr);

void Jump_ToApp(uint32_t app_addr);

#endif