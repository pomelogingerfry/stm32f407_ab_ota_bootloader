#include "boot_jump.h"
#include "stm32f407xx.h"

typedef void (*AppResetHandeler)(void);

uint8_t App_IsValid(uint32_t app_addr){

    uint32_t msp=*(volatile uint32_t*)app_addr;

    if(msp>=0x20000000UL&&msp<=0x20030000UL){

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

