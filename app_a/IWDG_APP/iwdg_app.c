#include "iwdg_app.h"
#include "stm32f4xx_hal.h"

static TaskHeartbeat_ts task_heartbeat[TASK_ID_MAX];

void IWDG_RegInit(void)
{
    IWDG->KR=0x5555;
    IWDG->PR=0x04;
    IWDG->RLR=625;
    IWDG->KR=0xCCCC;
}

void IWDG_TaskHeat_Init(void)
{
    task_heartbeat[TASK_IDLE_MAIN].last_tick=HAL_GetTick();
    task_heartbeat[TASK_IDLE_MAIN].timeout_ms=50;

    task_heartbeat[TASK_OTA_RECV].last_tick=HAL_GetTick();
    task_heartbeat[TASK_OTA_RECV].timeout_ms=100;

    task_heartbeat[TASK_FLASH_WRITE].last_tick=HAL_GetTick();
    task_heartbeat[TASK_FLASH_WRITE].timeout_ms=800;

}

void TaskHeartbeat_Fresh(TaskId_te task_id)
{
    if(task_id>TASK_ID_MAX)
    {
        return;
    }
    task_heartbeat[task_id].last_tick=HAL_GetTick();

}

bool IWDG_CheckAllTask(void)
{
    uint32_t now=HAL_GetTick();
    for(uint8_t i=0;i<TASK_ID_MAX;i++)
    {
        if(now-task_heartbeat[i].last_tick>task_heartbeat[i].timeout_ms)
        {
            return false;
        }
        return true;
    }
}

void IWDG_FeedDog(void)
{
    IWDG->KR=0xAAAA;
}