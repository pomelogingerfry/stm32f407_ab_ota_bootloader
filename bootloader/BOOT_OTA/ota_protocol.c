#include "ota_protocol.h"
#include <string.h>
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart3;

uint32_t protocol_CRC32_clac(const uint8_t *data,uint32_t len)
{
    uint32_t crc=0xFFFFFFFF;
    for(uint32_t i=0;i<len;i++)
    {
        crc^=data[i];
        for(int a=0;a<8;a++)
        {
            crc=(crc>>1)^(0xEDB88320&(-(crc&1)));
        }
    }
    return ~crc;
}

void ota_send_ack(uint16_t seq,uint8_t status)
{
    uint8_t ack_buf[6]={0xAA,0xBB,0x02,(uint8_t)(seq>>8),(uint8_t)(seq&0xFF),status};
    HAL_UART_Transmit(&huart3,ack_buf,6,1000);
}

int ota_prase_frame(uint8_t *buf,uint16_t size,ota_frame_ts *out_frame)
{
    uint16_t len;
    uint32_t frame_crc;
    uint32_t calc_crc;

    if(size<OTA_FRAME_MIN_LEN)return ERR_LEN;
    if(buf[0]!=0xBB||buf[1]!=0xAA)return ERR_LEN;

    out_frame->head=OTA_FRAME_HEAD;
    out_frame->type=buf[2];
    out_frame->seq=(buf[3]<<8)|buf[4];
    len=(buf[5]<<8)|buf[6];

    if(len>MAX_DATA_LEN)return ERR_LEN;
    if(size!=(OTA_FRAME_MIN_LEN+len))return ERR_LEN;

    out_frame->len=len;
    if(len>0)
    {
        memcpy(out_frame->data,&buf[OTA_FRAME_DATA_OFF],len);
    }

    frame_crc=((uint32_t)buf[OTA_FRAME_DATA_OFF+len]<<24)|
              ((uint32_t)buf[OTA_FRAME_DATA_OFF+len+1]<<16)|
              ((uint32_t)buf[OTA_FRAME_DATA_OFF+len+2]<<8)|
              ((uint32_t)buf[OTA_FRAME_DATA_OFF+len+3]);

    calc_crc=crc32_clac(out_frame->data,len);
    if(calc_crc!=frame_crc)return ERR_CRC;

    return ERR_OK;
}
