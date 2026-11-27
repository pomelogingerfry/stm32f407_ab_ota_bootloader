#ifndef __OTA_PROTOCOL_H
#define __OTA_PROTOCOL_H
#include "stm32f4xx_hal.h"
#include "error_code.h"

#define OTA_HEADER_0 0xAA
#define OTA_HEADER_1 0xBB
#define OTA_MAX_DATA_LEN 1024

#define OTA_CMD_START 0x01
#define OTA_CMD_DATA 0x02
#define OTA_CMD_END 0x03
#define OTA_CMD_ACK 0x04


typedef struct __attribute__((packed))
{
    uint8_t cmd;
    uint16_t seq;
    uint16_t len;
    uint8_t data[OTA_MAX_DATA_LEN];
}OTA_Packet_ts;

extern volatile uint8_t ota_rx_flag;
extern OTA_Packet_ts ota_packet;

uint16_t OTA_Protocol_Parse(uint8_t *buf,uint16_t size);
void OTA_Protocol_SendAck(uint16_t seq,uint8_t status);

#endif
