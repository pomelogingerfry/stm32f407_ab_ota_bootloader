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
__attribute__((noreturn))static void Boot_Do_Jump(uint32_t sp,
    uint32_t pc,uint32_t vtor_addr)
    {
        __disable_irq();

        HAL_DeInit();

        SysTick->CTRL=0U;
        SysTick->LOAD=0U;
        SysTick->VAL=0U;

        SCB->VTOR=vtor_addr;

        __DSB();
        __ISB();

        __set_MSP(sp);

        void(*slot_reset_entry)(void)=(void(*)(void))pc;
        slot_reset_entry();
        while(1);
    }
    BOOT_JUMP_RET Boot_jump_Exec(uint32_t slot_base)
    {
        uint32_t slot_sp=*(volatile uint32_t *)slot_base;
        uint32_t slot_pc=*(volatile uint32_t *)(slot_base+4U);
        if((slot_sp<0x20000000UL)||(slot_sp>0x20020000UL))
        {
            return JUMP_ERR_SP_INVALID;
        }
        if((slot_pc&0x1UL)==0UL)
        {
            return JUMP_ERR_PC_THUMB;
        }
        uint32_t actual_pc=slot_pc&0xFFFFFFFEUL;
        if((actual_pc<slot_base)||(actual_pc>=slot_base+SLOT_SIZE))
        {
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
    if((slot_sp<0x20000000UL)||slot_sp>=0x20020000UL)
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