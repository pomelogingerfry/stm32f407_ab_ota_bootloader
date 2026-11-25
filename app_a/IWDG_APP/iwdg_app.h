#ifndef __IWDG_HEARTBEAT_H
#define __IWDG_HEARTBEAT_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    TASK_IDLE_MAIN=0,
    TASK_OTA_RECV,
    TASK_FLASH_WRITE,
    TASK_ID_MAX
}TaskId_te;

typedef struct 
{
    uint32_t last_tick;
    uint32_t timeout_ms;

}TaskHeartbeat_ts;

void IWDG_RegInit(void);

void IWDG_TaskHeartbeat_Init(void);

void TaskHeartbeat_Fresh(TaskId_te task_id);

bool IWDG_CheckAllTask(void);

void IWDG_FeedDog(void);

#endif