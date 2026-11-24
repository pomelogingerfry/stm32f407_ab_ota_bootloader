#include "ring_buffer.h"
#include <string.h>

void RingBuffer_Init(Ring_Buffer_ts *tp)
{
    if(tp==NULL)return;
    tp->wr=0;
    tp->rd=0;
}

uint32_t RingBuffer_Available(Ring_Buffer_ts *tp)
{
    if(tp==NULL)return 0;
    return tp->wr-tp->rd;
}

uint32_t RingBuffer_FreeSpace(Ring_Buffer_ts *tp)
{
    if(tp==NULL)return 0;
    return (RING_BUF_SIZE-1)-(tp->wr-tp->rd);
}

bool RingBuffer_Write(Ring_Buffer_ts *tp,const uint8_t *data,uint32_t len)
{
    if(tp==NULL||data==NULL||len==0)return false;
    uint32_t free=RingBuffer_FreeSpace(tp);
    if(free<len)return false;

    uint32_t wr=tp->wr;
    uint32_t idx=wr&(RING_BUF_SIZE-1);
    uint32_t to_end=RING_BUF_SIZE-idx;

    if(len<=to_end)
    {
        memcpy(&tp->buffer[idx],data,len);
    }
    else
    {
        memcpy(&tp->buffer[idx],data,to_end);
        memcpy(&tp->buffer[0],data+to_end,len-to_end);
    }
    __DMB();
    tp->wr=wr+len;
    return true;
}

uint32_t RingBuffer_Read(Ring_Buffer_ts *tp,uint8_t *desk,uint32_t max_len)
{
    if(tp==NULL||desk==NULL||max_len==0)
    return 0;
    uint32_t avail=RingBuffer_Available(tp);
    if(avail==0)
    return 0;

    uint32_t read_len=(avail>max_len)?max_len:avail;
    uint32_t rd=tp->rd;
    uint32_t idx=rd&(RING_BUF_SIZE-1);
    uint32_t to_end=RING_BUF_SIZE-idx;

    if(read_len<=to_end)
    {
        memcpy(desk,&tp->buffer[idx],read_len);
    }
    else
    {
        memcpy(desk,&tp->buffer[idx],to_end);
        memcpy(desk+to_end,&tp->buffer[0],read_len-to_end);
    }
    __DMB();
    tp->rd=rd+read_len;
    return read_len;
}