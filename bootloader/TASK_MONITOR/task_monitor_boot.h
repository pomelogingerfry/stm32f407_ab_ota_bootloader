#ifndef __TASK_MONITOR_BOOT_H
#define __TASK_MONITOR_BOOT_H

#include "stm32f4xx_hal.h"

typedef enum{
    TASK_ID_RINGBUF=0,
    TASK_ID_SM_RUN,
    TASK_ID_SM_RECEIVE,
    TASK_ID_PROTOCOL_PARSE,
    TASK_ID_MAX
}TaskId_te;

#define MONITOR_TIMEOUT_MS 2000U

void TaskMonitor_Init(void);

void TaskMonitor_Feed(TaskId_te task_id);

void TaskMonitor_TimeoutCheck(void);

#endif