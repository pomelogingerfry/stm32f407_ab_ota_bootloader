#include "interface_bootloader.h"

uint8_t rec_buf[BOOTLOADER_REC_LEN]={0};

void int_bootloader_init(void){
//首先进行初始化
__HAL_UART_CLEAR_OREFLAG(&huart3);//清空OREFLAG
__HAL_UART_CLEAR_IDLEFLAG(&huart3);//清空IDLEFLAG

    HAL_UARTEx_ReceiveToIdle_IT(&huart3,&rec_buf,BOOTLOADER_REC_LEN);


}