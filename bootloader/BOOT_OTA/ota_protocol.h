#ifndef __OTA_PROTOCOL_H
#define __OTA_PROTOCOL_H
#include <stdint.h>
#include "error_code.h"

#define OTA_FRAME_HEAD 0xAABB
#define OTA_FRAME_MIN_LEN 11
#define OTA_FRAME_DATA_OFF 7
#define MAX_DATA_LEN 1024

#pragma pack(push,1)
typedef struct
{
    uint16_t head;
    uint8_t type;
    uint16_t seq;
    uint16_t len;
    uint8_t data[MAX_DATA_LEN];
    uint32_t crc32;
}ota_frame_ts;
#pragma pack(pop)

uint32_t crc32_clac(const uint8_t *data,uint32_t len);

int ota_prase_frame(uint8_t *buf,uint16_t size,ota_frame_ts *out_frame);

void ota_send_ack(uint16_t seq,uint8_t status);

#endif
