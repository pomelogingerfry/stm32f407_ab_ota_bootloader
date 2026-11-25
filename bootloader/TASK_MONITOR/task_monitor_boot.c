#include "task_monitor_boot.h"
#include <string.h>

extern UART_HandleTypeDef huart3;

static uint32_t last_feed_tick[TASK_ID_MAX]={0};
static const char *task_name[TASK_ID_MAX]={
    "RINGBUF","SM_RUN","SM_RECEIVE","PROTOCOL_PARSE"
};

void TaskMonitor_Init(void)
{
    for(uint8_t i=0;i<TASK_ID_MAX;i++)
    {
        last_feed_tick[i]=0;
    }
}

void TaskMonitor_Feed(TaskId_te task_id)
{
    if(task_id>TASK_ID_MAX)
    {
        return;
    }
    __disable_irq();
    last_feed_tick[task_id]=HAL_GetTick();
    __enable_irq();
}

void TaskMonitor_TimeoutCheck(void)
{
    uint32_t now=HAL_GetTick();
    uint8_t has_fault=0;
    for(uint8_t i=0;i<TASK_ID_MAX;i++)
    {
        uint32_t last_tick;
        __disable_irq();
        last_tick=last_feed_tick[i];
        __enable_irq();
        if(now-last_tick>MONITOR_TIMEOUT_MS)
        {
            has_fault=1;
            __disable_irq();
            const char *head_messa="\r\n[BOOT FAULT] TASK TIMEOUT:";
            HAL_UART_Transmit(&huart3,(uint8_t*)head_messa,strlen(head_messa),100);
            HAL_UART_Transmit(&huart3,(uint8_t*)task_name[i],strlen(task_name[i]),100);

            const char *end_messa="\r\n[BOOT FAULT] SOFTWARE RESET!\r\n";
            HAL_UART_Transmit(&huart3,(uint8_t*)end_messa,strlen(end_messa),100);
            HAL_Delay(100);
            NVIC_SystemReset();
        }
    }
}