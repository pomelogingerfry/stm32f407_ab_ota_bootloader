#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include<stdint.h>
#include<stdbool.h>

#define RING_BUF_SIZE 4096

typedef struct{
    uint8_t buffer[RING_BUF_SIZE];
    volatile uint32_t wr;
    volatile uint32_t rd;
}Ring_Buffer_ts;

void RingBuffer_Init(Ring_Buffer_ts *tp);

bool RingBuffer_Write(Ring_Buffer_ts *tp,const uint8_t *data,uint32_t len);

uint32_t RingBuffer_Read(Ring_Buffer_ts *tp,uint8_t *dest,uint32_t max_len);

uint32_t RingBuffer_Available(Ring_Buffer_ts *tp);

uint32_t RingBuffer_FreeSpace(Ring_Buffer_ts *tp);


#endif