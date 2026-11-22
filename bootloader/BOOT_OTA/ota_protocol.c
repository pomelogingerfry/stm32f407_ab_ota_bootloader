#include "ota_protocol.h"
#include <string.h>

extern UART_HandleTypeDef huart3;

volatile uint8_t ota_rx_flag=0;

OTA_Packet_ts ota_packet;

typedef enum{
    RX_WAIT_H0,RX_WAIT_H1,RX_WAIT_CMD,RX_WAIT_SEQ_H,RX_WAIT_SEQ_L,
    RX_WAIT_LEN_H,RX_WAIT_LEN_L,RX_WAIT_DATA,RX_WAIT_CRC
}RXstate_te;

static RXstate_te rx_state=RX_WAIT_H0;
static uint16_t rx_idx=0;
static uint32_t pack_crc_calc=0;
static uint8_t crc_buf[4];
uint8_t crc_idx=0;

static uint32_t protocol_CRC32_clac(uint32_t crc,uint8_t data)
{
        crc^=data;
        for(int a=0;a<8;a++)
        {
            crc=(crc>>1)^(0xEDB88320&(-(crc&1)));
        }
    
    return ~crc;
}

void OTA_Protocol_Parse(uint8_t *buf,uint16_t size)
{
    for(uint16_t i=0;i<size;i++)
    {
        uint8_t byte=buf[i];

        switch(rx_state){
            case RX_WAIT_H0:
            if(byte==OTA_HEADER_0)rx_state=RX_WAIT_H1;
            break;

            case RX_WAIT_H1:
            if(byte==OTA_HEADER_1){rx_state=RX_WAIT_CMD;
            pack_crc_calc=0xFFFFFFFF;
            }else if(byte==OTA_HEADER_0)
            {
                rx_state=RX_WAIT_H1;
            }else{
                rx_state=RX_WAIT_H0;
            }
            break;

            case RX_WAIT_CMD:
            ota_packet.cmd=byte;
            pack_crc_calc=protocol_CRC32_clac(pack_crc_calc,byte);
            rx_state=RX_WAIT_SEQ_H;
            break;

            case RX_WAIT_SEQ_H:
            ota_packet.seq=(uint16_t)byte<<8;
            pack_crc_calc=protocol_CRC32_clac(pack_crc_calc,byte);
            rx_state=RX_WAIT_SEQ_L;
            break;

            case RX_WAIT_SEQ_L:
            ota_packet.seq|=byte;
           pack_crc_calc=protocol_CRC32_clac(pack_crc_calc,byte);
            rx_state=RX_WAIT_LEN_H;
            break;

            case RX_WAIT_LEN_H:
            ota_packet.len=(uint16_t)byte<<8;
            pack_crc_calc=protocol_CRC32_clac(pack_crc_calc,byte);
            rx_state=RX_WAIT_LEN_L;
            break;

            case RX_WAIT_LEN_L:
            ota_packet.len|=byte;
            pack_crc_calc=protocol_CRC32_clac(pack_crc_calc,byte);
            if(ota_packet.len>OTA_MAX_DATA_LEN){
                rx_state=(byte==OTA_HEADER_0)?RX_WAIT_H1:RX_WAIT_H0;
            }else if(ota_packet.len==0)
            {
                rx_idx=0;
                crc_idx=0;
                rx_state=RX_WAIT_CRC;
            }else
            {
                rx_idx=0;
                rx_state=RX_WAIT_DATA;
            }
            break;

            case RX_WAIT_DATA:
            ota_packet.data[rx_idx++]=byte;
             pack_crc_calc=protocol_CRC32_clac(pack_crc_calc,byte);
             if(rx_idx>=ota_packet.len)
             {
                crc_idx=0;
                rx_state=RX_WAIT_CRC;
             }
             break;

             case RX_WAIT_CRC:
             crc_buf[crc_idx++]=byte;
             if(crc_idx>=4)
             {
                uint32_t recv_crc=(crc_buf[0]<<24)|(crc_buf[1]<<16)
                |(crc_buf[2]<<8)|crc_buf[3];
                if(recv_crc==(~pack_crc_calc))
                {
                    ota_rx_flag=1;
                    OTA_Protocol_SendAck(ota_packet.seq,0);
                }else{
                    OTA_Protocol_SendAck(ota_packet.seq,1);
                }
                rx_state=(byte==OTA_HEADER_0)?RX_WAIT_H1:RX_WAIT_H0;
             }
             break;
        }
    }
}
void OTA_Protocol_SendAck(uint16_t seq,uint8_t status)
{
    uint8_t ack[6];
    ack[0]=OTA_HEADER_0;
    ack[1]=OTA_HEADER_1;
    ack[2]=OTA_CMD_ACK;
    ack[3]=status;
    ack[4]=(uint8_t)(seq>>8);
    ack[5]=(uint8_t)(seq&0xFF);

    HAL_UART_Transmit(&huart3,ack,sizeof(ack),100);
}