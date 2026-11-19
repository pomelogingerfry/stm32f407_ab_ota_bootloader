#include "hardfault_capture.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

volatile FaultSnapshot_ts glo_faul_snapshot_buf __attribute__((section(".bss.noinit"),used));
#ifdef BOOTLOADER
static uint8_t FaultSnapshot_SaveToFlash(void)
{
    FLASH_EraseInitTypeDef erase_settings={0};
    uint32_t erase_err=0U;
    uint8_t ret=0U;

    HAL_FLASH_Unlock();

    erase_settings.TypeErase=FLASH_TYPEERASE_SECTORS;
    erase_settings.Sector=FLASH_SECTOR_10;
    erase_settings.NbSectors=1U;
    erase_settings.VoltageRange=FLASH_VOLTAGE_RANGE_3;

    if(HAL_FLASHEx_Erase(&erase_settings,&erase_err)!=HAL_OK)
    {
        goto exit_savetoflash;
    }
    uint32_t *source_p=(uint32_t*)&glo_faul_snapshot_buf;
    uint32_t dest_addr=FAULT_BASE;
    size_t word_cnt=sizeof(FaultSnapshot_ts)/sizeof(uint32_t);

    for(size_t i=0U;i<word_cnt;i++)
    {
        if((dest_addr<FAULT_BASE)||(dest_addr>=(FAULT_BASE+FAULT_SIZE)))
        {
            goto exit_savetoflash;
        }
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,dest_addr,
            source_p[i])!=HAL_OK)
            {
                goto exit_savetoflash;
            }
            dest_addr+=4U;
    }
    ret=1U;
    exit_savetoflash:
    HAL_FLASH_Lock();
    return ret;
}
#endif
#define FAULT_RAM_START    0x20000000UL
#define FAULT_RAM_END      0x20020000UL
#define FAULT_FRAME_WORDS  8U

static uint8_t Fault_StackFrameIsValid(const uint32_t *sp)
{
    uint32_t sp_addr=(uint32_t)sp;

    if((sp_addr<FAULT_RAM_START)||
       (sp_addr>(FAULT_RAM_END-FAULT_FRAME_WORDS*4U))||
       ((sp_addr&0x3U)!=0U))
    {
        return 0U;
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
        "MOV R1,LR     \n"
        "B HardFault_Capture \n"
    );
}
void Error_Handler(void)
{
    Fault_SaveFromErrorHandler((uint32_t)__builtin_return_address(0));

    __disable_irq();

    while(1);
}

__attribute__((used,noinline,noreturn)) void HardFault_Capture(uint32_t *sp,uint32_t exc_return)
{
    __disable_irq();

    glo_faul_snapshot_buf.sp=(uint32_t)sp;
    glo_faul_snapshot_buf.reserved[0]=exc_return;

    if(Fault_StackFrameIsValid(sp))
    {
        glo_faul_snapshot_buf.r0=sp[0];
        glo_faul_snapshot_buf.r1=sp[1];
        glo_faul_snapshot_buf.r2=sp[2];
        glo_faul_snapshot_buf.r3=sp[3];
        glo_faul_snapshot_buf.r12=sp[4];
        glo_faul_snapshot_buf.lr=sp[5];
        glo_faul_snapshot_buf.pc=sp[6];
        glo_faul_snapshot_buf.xpsr=sp[7];
    }
    else
    {
        glo_faul_snapshot_buf.r0=0U;
        glo_faul_snapshot_buf.r1=0U;
        glo_faul_snapshot_buf.r2=0U;
        glo_faul_snapshot_buf.r3=0U;
        glo_faul_snapshot_buf.r12=0U;
        glo_faul_snapshot_buf.lr=0U;
        glo_faul_snapshot_buf.pc=0U;
        glo_faul_snapshot_buf.xpsr=0U;
    }

    glo_faul_snapshot_buf.cfsr=SCB->CFSR;
    glo_faul_snapshot_buf.hfsr=SCB->HFSR;
    glo_faul_snapshot_buf.mmfar=SCB->MMFAR;
    glo_faul_snapshot_buf.bfar=SCB->BFAR;
    glo_faul_snapshot_buf.afsr=SCB->AFSR;

    SCB->CFSR=glo_faul_snapshot_buf.cfsr;
    SCB->HFSR=glo_faul_snapshot_buf.hfsr;

    glo_faul_snapshot_buf.magic=FAULT_MAGIC_HARD;
    __DSB();

    NVIC_SystemReset();
    while(1);
}
void Fault_SaveFromErrorHandler(uint32_t fault_pc)
{
    __disable_irq();
    uint32_t msp_val;
    __asm volatile ("MRS %0,MSP":"=r"(msp_val));
    glo_faul_snapshot_buf.sp=msp_val;
    glo_faul_snapshot_buf.pc=fault_pc;
    glo_faul_snapshot_buf.lr=fault_pc;
    glo_faul_snapshot_buf.reserved[0]=0U;

    glo_faul_snapshot_buf.cfsr=SCB->CFSR;
    glo_faul_snapshot_buf.hfsr=SCB->HFSR;
    glo_faul_snapshot_buf.mmfar=SCB->MMFAR;
    glo_faul_snapshot_buf.bfar=SCB->BFAR;
    glo_faul_snapshot_buf.afsr=SCB->AFSR;

    glo_faul_snapshot_buf.magic=FAULT_MAGIC_SOFT;
    __DSB();

    NVIC_SystemReset();
    while(1);
}
#ifdef BOOTLOADER
uint8_t FaultSnapshot_CheckAndSavetoflash(void)
{
    const FaultSnapshot_ts *flash_p=(const FaultSnapshot_ts*)FAULT_BASE;

    if((glo_faul_snapshot_buf.magic==FAULT_MAGIC_HARD)
        ||(glo_faul_snapshot_buf.magic==FAULT_MAGIC_SOFT))
        {
         if(FaultSnapshot_SaveToFlash())
         {
            if((flash_p->magic==glo_faul_snapshot_buf.magic)&&
               (flash_p->pc==glo_faul_snapshot_buf.pc)&&
               (flash_p->sp==glo_faul_snapshot_buf.sp))
            {
                glo_faul_snapshot_buf.magic=0U;
                return 1U;
            }
         }
        }
        return 0U;
}
void FaultSnapshot_LoadAndPrint(void)
{
    FaultSnapshot_ts *snap_p=(FaultSnapshot_ts*)FAULT_BASE;
    if((snap_p->magic!=FAULT_MAGIC_HARD)&&(snap_p->magic!=FAULT_MAGIC_SOFT))
    {
        return;
    }

printf("\r\n======HardFault Snapshot Found======\r\n");
if(snap_p->magic==FAULT_MAGIC_SOFT)
{
    printf("Fault_Type:Software(Error_Handler)\r\n");
}
else
{
    printf("Fault_Type:Hardware(HardFault_Handler)\r\n");
}
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
    printf("EXC_RETURN:0x%08X\r\n",snap_p->reserved[0]);
printf("====================================\r\n\r\n");
    printf("Snapshot kept at 0x%08X, call FaultSnapshot_Clear() to erase\r\n",
           (uint32_t)FAULT_BASE);
}
uint8_t FaultSnapshot_Clear(void)
{
    FLASH_EraseInitTypeDef erase_settings={0};
    uint32_t erase_err=0U;
    uint8_t ret=0U;
    HAL_FLASH_Unlock();

    erase_settings.TypeErase=FLASH_TYPEERASE_SECTORS;
    erase_settings.Sector=FLASH_SECTOR_10;
    erase_settings.NbSectors=1U;
    erase_settings.VoltageRange=FLASH_VOLTAGE_RANGE_3;

    if(HAL_FLASHEx_Erase(&erase_settings,&erase_err)==HAL_OK)
    {
        ret=1U;
    }
    HAL_FLASH_Lock();
    return ret;
}
#endif

