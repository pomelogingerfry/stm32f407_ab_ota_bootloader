#include "hardfault_capture.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
 
FaultSnapshot_s glo_faul_snapshot_buf;

static uint8_t FaultSnapshot_EraseSector(void)
{
    FLASH_EraseInitTypeDef erase_settings={0};

    uint32_t erase_err=0U;

    HAL_FLASH_Unlock();

    erase_settings.TypeErase=FLASH_TYPEERASE_SECTORS;
    erase_settings.Sector=FLASH_SECTOR_10;
    erase_settings.NbSectors=1U;
    erase_settings.VoltageRange=FLASH_VOLTAGE_RANGE_3;

    if(HAL_FLASHEx_Erase(&erase_settings,&erase_err)!=HAL_OK)
    {
        HAL_FLASH_Lock();
        return 0U;
    }
        HAL_FLASH_Lock();
        return 1U;
}

static uint8_t FaultSnapshot_WriteWord(uint32_t addr,uint32_t data)
{
    if((addr<FAULT_BASE)||(addr>FAULT_BASE))
    {
        return 0U;
    }
    HAL_FLASH_Unlock();
    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,addr,data)!=HAL_OK)
    {
        HAL_FLASH_Lock();
        return 0U;
    }
        HAL_FLASH_Lock();
        return 1U;
}
static uint8_t FaultSnapshot_SaveToFlash(void)
{
    if(!FaultSnapshot_EraseSector())
    {
        return 0U;
    }
    uint32_t *source_p=(uint32_t*)&glo_faul_snapshot_buf;
    uint32_t dest_addr=FAULT_BASE;
    size_t word_cnt=sizeof(FaultSnapshot_s)/sizeof(uint32_t);
    for(size_t i=0U;i<word_cnt;i++)
    {
        if(!FaultSnapshot_WriteWord(dest_addr,source_p[i]))
        {
            return 0U;
        }
        dest_addr+=4U;
    }
            return 1U;
}

__attribute__((naked))void HardFault_Handler(void)
{
    __asm volatile(
        "TST LR,#4  \n"
        "ITE EQ     \n"
        "MRSEQ R0,MSP  \n"
        "MRSNE R0,PSP  \n"
        "B HardFault_Capture \n"
    );
}

__attribute__((used,noinline)) void HardFault_Capture(uint32_t *sp)
{ 
    glo_faul_snapshot_buf.r0=sp[0];
    glo_faul_snapshot_buf.r1=sp[1];
    glo_faul_snapshot_buf.r2=sp[2];
    glo_faul_snapshot_buf.r3=sp[3];
    glo_faul_snapshot_buf.r12=sp[4];
    glo_faul_snapshot_buf.lr=sp[5];
    glo_faul_snapshot_buf.pc=sp[6];
    glo_faul_snapshot_buf.xpsr=sp[7];
    glo_faul_snapshot_buf.sp=(uint32_t)sp;

    glo_faul_snapshot_buf.cfsr=SCB->CFSR;
    glo_faul_snapshot_buf.hfsr=SCB->HFSR;
    glo_faul_snapshot_buf.mmfar=SCB->MMFAR;
    glo_faul_snapshot_buf.bfar=SCB->BFAR;
    glo_faul_snapshot_buf.afsr=SCB->AFSR;
    glo_faul_snapshot_buf.magic=FAULT_SNAPSHOT_MAGIC;

    (void)FaultSnapshot_SaveToFlash();
    NVIC_SystemReset();
}
void Fault_SaveFromErrorHandler(void)
{
    uint32_t dummy_sp[8]={0};
    glo_faul_snapshot_buf.r0=dummy_sp[0];
    glo_faul_snapshot_buf.r1=dummy_sp[1];
    glo_faul_snapshot_buf.r2=dummy_sp[2];
    glo_faul_snapshot_buf.r3=dummy_sp[3];
    glo_faul_snapshot_buf.r12=dummy_sp[4];
    glo_faul_snapshot_buf.lr=dummy_sp[5];
    glo_faul_snapshot_buf.pc=dummy_sp[6];
    glo_faul_snapshot_buf.xpsr=dummy_sp[7];
    glo_faul_snapshot_buf.sp=0U;
    glo_faul_snapshot_buf.cfsr=SCB->CFSR;
    glo_faul_snapshot_buf.hfsr=SCB->HFSR;
    glo_faul_snapshot_buf.mmfar=SCB->MMFAR;
    glo_faul_snapshot_buf.bfar=SCB->BFAR;
    glo_faul_snapshot_buf.magic=FAULT_SNAPSHOT_MAGIC;

    (void)FaultSnapshot_SaveToFlash();
    NVIC_SystemReset();
}
#ifdef BOOTLOADER 
void FaultSnapshot_LoadAndPrint(void)
{
    FaultSnapshot_s *snap_p=(FaultSnapshot_s *)FAULT_BASE;
    if(snap_p->magic!=FAULT_SNAPSHOT_MAGIC)
    {
        return;
    }
printf("\r\n======HardFault Snapshot Found======\r\n");
    printf("PC:0x%08X\r\n",snap_p->pc);
    printf("LR:0x%08X\r\n",snap_p->lr);
    printf("SP:0x%08X\r\n",snap_p->sp);
    printf("XPSR:0x%08X\r\n",snap_p->xpsr);
    printf("R0:0x%08X\r\n",snap_p->r0);
    printf("R1:0x%08X\r\n",snap_p->r1);
    printf("R2:0x%08X\r\n",snap_p->r2);
    printf("R3:0x%08X\r\n",snap_p->r3);
    printf("CFSR:0x%08X\r\n",snap_p->cfsr);
    printf("HFSR:0x%08X\r\n",snap_p->hfsr);
    printf("MMFAR:0x%08X\r\n",snap_p->mmfar);
    printf("BFAR:0x%08X\r\n",snap_p->bfar);
    printf("AFSR:0x%08X\r\n",snap_p->afsr);
printf("====================================\r\n\r\n");

    FaultSnapshot_Clear();
}
uint8_t FaultSnapshot_Clear(void)
{
    return FaultSnapshot_EraseSector();
}
#endif

