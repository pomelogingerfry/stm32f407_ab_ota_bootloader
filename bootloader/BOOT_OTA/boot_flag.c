#include "boot_flag.h"
#include "flash_partition.h"
#include "ota_protocol.h"
#include <string.h>
#include "flash_driver.h"
#include <stddef.h>

extern FLASH_STATUS_TYPE Flash_EraseSector(uint32_t addr);
extern FLASH_STATUS_TYPE Flash_Write(uint32_t dest_addr,uint8_t *buf,
uint32_t len_byte,uint32_t *next_wr_addr);
extern FLASH_STATUS_TYPE Flash_Read(uint32_t source_addr,uint8_t *dsk_buf,uint32_t len);

static uint32_t bootflag_CRC32_calc(const boot_flag_ts *flag)
{
    uint32_t crc=0xFFFFFFFF;
    const uint8_t *p=(const uint8_t*)flag;
    uint32_t len=offsetof(boot_flag_ts,flag_crc);

    for(uint32_t i=0;i<len;i++)
    {
        crc^=p[i];
        for(int a=0;a<8;a++)
        {
            if(crc&1)crc=(crc>>1)^0xEDB88320;
            else crc>>=1;
        }
    }
    return ~crc;
}

void BootFlag_Read(boot_flag_ts *flag)
{
    boot_flag_ts flag_a,flag_b;

    Flash_Read(META_BASE,(uint8_t*)&flag_a,sizeof(boot_flag_ts));
    Flash_Read(META_MIRROR_BASE,(uint8_t*)&flag_b,sizeof(boot_flag_ts));
    uint8_t valid_a=(flag_a.magic==OTA_FLAG_MAGIC)
    &&(flag_a.flag_crc==bootflag_CRC32_calc(&flag_a));
    uint8_t valid_b=(flag_b.magic==OTA_FLAG_MAGIC)
    &&(flag_b.flag_crc==bootflag_CRC32_calc(&flag_b));

    if(valid_a&&valid_b){
        *flag=(flag_a.seq>flag_b.seq)?flag_a:flag_b;
    }else if(valid_a)
    {
        *flag=flag_a;
    }else if(valid_b)
    {
        *flag=flag_b;
    }else 
    {
        memset(flag,0,sizeof(boot_flag_ts));
        flag->magic=OTA_FLAG_MAGIC;
        flag->active_slot=0;
        flag->pending_slot=0xFF;
        flag->state=FLAG_STATE_IDLE;;
        flag->seq=0;
    }
}

void BootFlag_Write(boot_flag_ts *flag)
{
    boot_flag_ts current;
    BootFlag_Read(&current);
    flag->magic=OTA_FLAG_MAGIC;
    flag->seq=current.seq+1;

    memset(flag->reserved,0,sizeof(flag->reserved));

    flag->flag_crc=bootflag_CRC32_calc(flag);
    
    uint32_t write_addr=META_BASE;
    if(current.seq>0)
    {
        boot_flag_ts flag_a;
        Flash_Read(META_BASE,(uint8_t*)&flag_a,sizeof(boot_flag_ts));
        if(flag_a.seq==current.seq)
        {
            write_addr=META_MIRROR_BASE;
        }
    }
    Flash_EraseSector(write_addr);
    uint32_t next_addr;
    Flash_Write(write_addr,(uint8_t*)flag,sizeof(boot_flag_ts),&next_addr);
}