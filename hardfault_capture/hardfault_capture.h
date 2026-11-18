#ifndef __FAULT_CAPTURE_H
#define __FAULT_CAPTURE_H
#include "stm32f4xx_hal.h"
#include "flash_partition.h"

typedef struct __attribute((packed))
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;

    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t afsr;
    uint32_t sp;
    uint32_t magic;
    uint32_t reserved[4];

}FaultSnapshot_s;

#define FAULT_SNAPSHOT_MAGIC 0xABCDDCBAUL

extern FaultSnapshot_s glo_faul_snapshot_buf;

void HardFault_Capture(uint32_t *sp);

void Fault_SaveFromErrorHandler(void);

#ifdef BOOTLOADER

uint8_t FaultSnapshot_Clear(void);

void FaultSnapshot_LoadAndPrint(void);

#endif

#endif

