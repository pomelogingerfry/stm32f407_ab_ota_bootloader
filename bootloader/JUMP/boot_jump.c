#include "boot_jump.h"
/**
typedef void (*AppResetHandeler)(void);

uint8_t App_IsValid(uint32_t app_addr){

    uint32_t msp=*(volatile uint32_t*)app_addr;

    if(msp>=0x20000000UL&&msp<=0x20020000UL){

        return 1;
    }

    return 0;
}
void Jump_ToApp(uint32_t app_adr){

    AppResetHandeler app_reset;

    uint32_t msp;

    msp=*(volatile uint32_t*)app_adr;

    app_reset=(AppResetHandeler)(volatile uint32_t*)(app_adr+4U);

    __disable_irq();

    NVIC->ICER[0]=NULL;
    NVIC->ICER[1]=NULL;

    NVIC->ICPR[0]=NULL;
    NVIC->ICPR[1]=NULL;

    SysTick->CTRL=0;

    __set_MSP(msp);
    
    app_reset();
}
*/
    void Boot_Do_Jump(uint32_t sp,
    uint32_t pc,uint32_t vtor_addr)
    {
       
       for(uint8_t i=0;i<8;i++)
       {
        NVIC->ICER[i]=0xFFFFFFFF;
        NVIC->ICPR[i]=0xFFFFFFFF;
       }
       SysTick->CTRL=0;
        __disable_irq();
        __set_MSP(sp);

        SCB->VTOR=vtor_addr;
        __DSB();
        __ISB();
volatile uint32_t addr=pc;
        typedef void (*resetfunc)(void);
        resetfunc resetentry=(resetfunc)addr;
        printf("11\r\n");
        resetentry();

        while(1)
        {
printf("33\r\n");
HAL_Delay(5000);
        }
    }
    BOOT_JUMP_RET Boot_jump_Exec(uint32_t slot_base)
    {
        printf("entered jump exec\r\n");
        uint32_t slot_sp=*(volatile uint32_t *)slot_base;
        uint32_t slot_pc=*(volatile uint32_t *)(slot_base+4U);
        if((slot_sp<0x20000000UL)||(slot_sp>=0x20020000UL))
        {
            printf("1\r\n");
            return JUMP_ERR_SP_INVALID;
        }
        if((slot_pc&0x1UL)==0UL)
        {
            printf("2\r\n");
            return JUMP_ERR_PC_THUMB;
        }
        uint32_t actual_pc=slot_pc&0xFFFFFFFEUL;
        if((actual_pc<slot_base)||(actual_pc>=slot_base+SLOT_SIZE))
        {
            printf("3\r\n");
            return JUMP_ERR_PC_OUT_RANGE;
        }
        Boot_Do_Jump(slot_sp,slot_pc,slot_base);
        return JUMP_OK;
    }

void Meta_Read(OtaMetaTypeDef *meta)
{
    *meta=*(volatile OtaMetaTypeDef*)META_BASE;
}
void Meta_Write(OtaMetaTypeDef *meta)
{
    uint32_t dummy_next;
    Flash_EraseSector(META_BASE);
    Flash_Write(META_BASE,(uint8_t*)meta,sizeof(OtaMetaTypeDef),&dummy_next);
    
}
BOOT_JUMP_RET Check_App_Valid(uint32_t app_addr)
{
    uint32_t slot_sp=*(volatile uint32_t*)app_addr;
    uint32_t slot_pc=*(volatile uint32_t*)(app_addr+4U);
    printf("sp:0x%08X,pc:0x%08X\r\n",slot_sp,slot_pc);
    if((slot_sp<0x20000000UL)||slot_sp>=0x20030000UL)
    {
        return JUMP_ERR_SP_INVALID;
    }
    if((slot_pc&0x1UL)==0UL)
    {
        return JUMP_ERR_PC_THUMB;
    }
    uint32_t actual_pc=slot_pc&0xFFFFFFFEUL;
    if((actual_pc<app_addr)||actual_pc>=app_addr+SLOT_SIZE)
    {
        return JUMP_ERR_PC_OUT_RANGE;
    }
    return JUMP_OK;
}