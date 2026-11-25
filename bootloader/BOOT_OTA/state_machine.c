#include "state_machine.h"
#include "boot_flag.h"
#include "ota_protocol.h"
#include "flash_driver.h"
#include "flash_partition.h"
#include <stdio.h>
#include <string.h>

extern FLASH_STATUS_TYPE FLASH_EraseSector(uint32_t sector_addr);
extern FLASH_STATUS_TYPE Flash_Write(uint32_t dest_addr,uint8_t *data,
    uint32_t len_byte,uint32_t *next_wr_addr);

typedef enum{
        SM_STATE_BOOT,
        SM_STATE_IDLE,
        SM_STATE_RECEIVING,
        SM_STATE_VERIFY,
        SM_STATE_SWITCH,
        SM_STATE_REJECT
    }Sm_State_te;

typedef struct
    {
        Sm_State_te state;
        uint32_t timeout_tick;
        uint32_t write_addr;
        uint32_t total_received;
        uint32_t expected_size;
        uint32_t expected_crc;
        uint16_t expect_seq;
    }Sm_Context_ts;

 static Sm_Context_ts sm_ctx;
 static boot_flag_ts sys_flag;
 
 static void Sm_ChangeState(Sm_State_te new_state)
 {
    sm_ctx.state=new_state;
    sm_ctx.timeout_tick=HAL_GetTick();
 }

static uint32_t Calc_FlashCRC32(uint32_t start_addr,uint32_t size)
{
    uint32_t crc=0xFFFFFFFF;
    uint8_t buffer[256];
    uint32_t offset=0;

    while(offset<size){
        uint32_t read_size=(size-offset>256)?256:(size-offset);
        Flash_Read(start_addr+offset,buffer,read_size);
        for(uint32_t i=0;i<8;i++){
            crc^=buffer[i];
            for(int a=0;a<read_size;a++){
                if(crc&1)crc=(crc>>1)^0xEDB88320;
                else crc>>=1;
            }
        }
        offset+=read_size;
    }
    return ~crc;
}

static void JumpToApp(uint32_t app_addr)
 {
    uint32_t app_sp=*(volatile uint32_t*)app_addr;
    uint32_t app_pc=*(volatile uint32_t*)(app_addr+4);

    if(app_sp<0x20000000UL||app_sp>0x20020000UL||
    (app_pc&0xFFFFFFFE)<APP_A_BASE||(app_pc&0xFFFFFFFE)>(APP_A_BASE+SLOT_SIZE))
    {
        printf("[SM]Error:Invalid App Vector!Stay in Bootloader...\r\n");
        while(1);
    }
    printf("[SM]Jumping to 0x%08X...\r\n",app_addr);
    HAL_Delay(100);

    __disable_irq();
    for(uint32_t i=0;i<8;i++)
        {
            NVIC->ICER[i]=0xFFFFFFFF;
            NVIC->ICPR[i]=0xFFFFFFFF;
        }
    HAL_DeInit();

    SysTick->CTRL=0;
    SysTick->LOAD=0;
    SysTick->VAL=0;

    SCB->VTOR=app_addr;
    __DSB();
    __ISB();
/*
    __set_MSP(app_sp);

    void (*app_entry)(void)=(void(*)(void))app_pc;
    app_entry();
*/
__asm volatile (
        "msr msp, %0 \n"
        "bx %1        \n"
        : : "r" (app_sp), "r" (app_pc) : "memory"
    );
 }


 void StateMachine_Init(void)
 {
    BootFlag_Read(&sys_flag);

    if(sys_flag.state==FLAG_STATE_TRIAL)
    {
        printf("[SM]Found Trial flag,Checking Slot %c...\r\n",
        sys_flag.pending_slot==SLOT_A?'A':'B');

        uint32_t trial_addr=(sys_flag.pending_slot==SLOT_A)?APP_A_BASE:APP_B_BASE;

        uint32_t sp=*(volatile uint32_t*)trial_addr;
        if(sp>=0x20000000&&sp<=0x20020000)
        {
            if(sys_flag.try_count>=3){
                printf("[SM]Trial Failed 3 times,Rolling Back!\r\n");
                sys_flag.pending_slot=SLOT_NONE;
                sys_flag.state=FLAG_STATE_IDLE;
                BootFlag_Write(&sys_flag);
            }else{
                sys_flag.try_count++;
                BootFlag_Write(&sys_flag);
                printf("[SM]Jumping to Trial App(Try:%d)...\r\n",sys_flag.try_count);
                JumpToApp(trial_addr);
            }
        }else{
            printf("[SM]Invalid Trial App,Rollback!\r\n");
            sys_flag.pending_slot=SLOT_NONE;
            sys_flag.state=FLAG_STATE_IDLE;
            BootFlag_Write(&sys_flag);
        }
    }
    Sm_ChangeState(SM_STATE_IDLE);
    printf("[SM]Bootloader Enter IDLE mode(3s timeout)...\r\n");
 }


 void StateMachine_Run(void)
 {
    uint32_t now=HAL_GetTick();

    switch(sm_ctx.state){
        case SM_STATE_IDLE:
        if(now-sm_ctx.timeout_tick>3000){
            uint32_t jump_addr=(sys_flag.active_slot==SLOT_A)?APP_A_BASE:APP_B_BASE;
            printf("[SM]IDLE Timeout,Jump to Active Slot %c\r\n",
            sys_flag.active_slot==SLOT_A?'A':'B');
            JumpToApp(jump_addr);
        }
        break;
        
        case SM_STATE_RECEIVING:
        if(now-sm_ctx.timeout_tick>5000){
            printf("[SM]Timeout Waiting for Data,Abort!\r\n");
            Sm_ChangeState(SM_STATE_REJECT);
        }
        break;

        case SM_STATE_VERIFY:
        printf("[SM]Verify Firmware...\r\n");

        if(sm_ctx.total_received!=sm_ctx.expected_size)
        {
            printf("[SM]Verify Failed:Size Mismatch(Expected:%u,Got:%u)\r\n",
            sm_ctx.expected_size,sm_ctx.total_received);
            Sm_ChangeState(SM_STATE_REJECT);
            break;
        }
        uint32_t calc_crc=Calc_FlashCRC32(sm_ctx.write_addr,sm_ctx.total_received);
        if(calc_crc!=sm_ctx.expected_crc){
            printf("[SM]Verify Failed:CRC Mismatch(Expected:0x%08X,Got:0x%08X)\r\n",
            sm_ctx.expected_crc,calc_crc);
            sm_ctx.total_received=0;
            Flash_EraseSector(sm_ctx.write_addr);
            Sm_ChangeState(SM_STATE_RECEIVING);
            break;
        }
        printf("[SM]Verify Success!Updating Flag to Trial\r\n");
        sys_flag.pending_slot=(sys_flag.active_slot==SLOT_A)?SLOT_B:SLOT_A;
        sys_flag.fw_size=sm_ctx.total_received;
        sys_flag.fw_crc32=sm_ctx.expected_crc;
        sys_flag.try_count=0;
        sys_flag.state=FLAG_STATE_TRIAL;
        BootFlag_Write(&sys_flag);
        Sm_ChangeState(SM_STATE_SWITCH);
        break;

        case SM_STATE_SWITCH:
        printf("[SM]Rebooting to Run New Firmware...\r\n");
        HAL_Delay(100);
        NVIC_SystemReset();
        break;

        case SM_STATE_REJECT:
        printf("[SM]Update Rejected,System Rebooting...\r\n");
        sys_flag.pending_slot=SLOT_NONE;
        sys_flag.state=FLAG_STATE_IDLE;
        BootFlag_Write(&sys_flag);
        HAL_Delay(1000);
        NVIC_SystemReset();
        break;

        default:
        break;
    }
 }


 void StateMachine_OneFrameReceived(void)
 {
    extern OTA_Packet_ts ota_packet;

    if(ota_packet.cmd==OTA_CMD_START&&sm_ctx.state==SM_STATE_IDLE)
    {
        if(ota_packet.len<8){
            printf("[SM]Invalid Start Payload\r\n");
            return;
        }
        sm_ctx.expected_size=(ota_packet.data[0]<<24)|(ota_packet.data[1]<<16)|
        (ota_packet.data[2]<<8)|(ota_packet.data[3]);
        
        sm_ctx.expected_crc=(ota_packet.data[4]<<24)|(ota_packet.data[5]<<16)|
        (ota_packet.data[6]<<8)|(ota_packet.data[7]);

        if(sm_ctx.expected_size>SLOT_SIZE){
            printf("[SM]Firmware Size Over Limit!\r\n");
            return;
        }
        uint8_t target_slot=(sys_flag.active_slot==SLOT_A)?SLOT_B:SLOT_A;
        sm_ctx.write_addr=(target_slot==SLOT_A)?APP_A_BASE:APP_B_BASE;

        printf("[SM]START cmd received,Size:%u,Target Slot:%c\r\n",
        sm_ctx.expected_size,target_slot==SLOT_A?'A':'B');
        printf("[SM]Erasing Target Slot(Blocking ~2-4s)...\r\n");

        Flash_EraseSector(sm_ctx.write_addr);
        Flash_EraseSector(sm_ctx.write_addr+128*1024);

        sm_ctx.total_received=0;
        sm_ctx.expect_seq=ota_packet.seq+1;

        Sm_ChangeState(SM_STATE_RECEIVING);

        printf("[SM]Erase done,Enter Receiving\r\n");
        return;
    }
    if(sm_ctx.state==SM_STATE_RECEIVING){
        if(ota_packet.cmd==OTA_CMD_DATA){
            if(ota_packet.seq<sm_ctx.expect_seq){
                OTA_Protocol_SendAck(ota_packet.seq,0);
                return;
            }
            if(ota_packet.seq!=sm_ctx.expect_seq){
                return;
            }
            if(sm_ctx.total_received+ota_packet.len>SLOT_SIZE){
                printf("[SM]Data Overflow!Abort\r\n");
                Sm_ChangeState(SM_STATE_REJECT);
                return;
            }
            uint32_t next_wr;
            if(Flash_Write(sm_ctx.write_addr+sm_ctx.total_received,ota_packet.data,
            ota_packet.len,&next_wr)!=FLASH_OK){
                printf("[SM]Flash Write Error at Offset %u!\r\n",sm_ctx.total_received);
                Sm_ChangeState(SM_STATE_REJECT);
                return;
            }
            sm_ctx.total_received+=ota_packet.len;
            sm_ctx.expect_seq++;
            sm_ctx.timeout_tick=HAL_GetTick();

            OTA_Protocol_SendAck(ota_packet.seq,0);

            if(ota_packet.seq%20==0){
                printf("[SM]Progress:%u/%u bytes\r\n",
                sm_ctx.total_received,sm_ctx.expected_size);
            }
        }
        else if(ota_packet.cmd==OTA_CMD_END){
            printf("[SM]END cmd received\r\n");
            Sm_ChangeState(SM_STATE_VERIFY);
        }
    }
 }
 