#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include "stm32f4xx_hal.h"

void StateMachine_Init(void);

void StateMachine_Run(void);

void StateMachine_OneFrameReceived(void);

#endif