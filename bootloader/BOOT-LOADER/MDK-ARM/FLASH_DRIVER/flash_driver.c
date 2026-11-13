#include "flash_driver.h"

uint32_t Flash_GetSector(uint32_t addr)
{
    if((addr>=0x08000000U)&&(addr<=0x08004000U))return FLASH_SECTOR_0;
    if((addr>=0x08004000U)&&(addr<=0x08008000U))return FLASH_SECTOR_1;
    if((addr>=0x08008000U)&&(addr<=0x0800C000U))return FLASH_SECTOR_2;
    if((addr>=0x0800C00U)&&(addr<=0x08010000U))return FLASH_SECTOR_3;
    if((addr>=0x08010000U)&&(addr<=0x08020000U))return FLASH_SECTOR_4;
    if((addr>=0x08020000U)&&(addr<=0x08040000U))return FLASH_SECTOR_5;
    if((addr>=0x08040000U)&&(addr<=0x08060000U))return FLASH_SECTOR_6;
    if((addr>=0x08060000U)&&(addr<=0x08080000U))return FLASH_SECTOR_7;
    if((addr>=0x08080000U)&&(addr<=0x080A0000U))return FLASH_SECTOR_8;
    if((addr>=0x080A0000U)&&(addr<=0x080C0000U))return FLASH_SECTOR_9;
    if((addr>=0x080C0000U)&&(addr<=0x080E0000U))return FLASH_SECTOR_10;
    if((addr>=0x080E0000U)&&(addr<=0x080F0000U))return FLASH_SECTOR_11;
    return 0xFFFFFFFFU;
}
Flash_Status_Type Flash_EraseSector(uint32_t sectoraddr)
{
    if((sectoraddr<FLASH_START_ADDR)||(sectoraddr>FLASH_END_ADDR))
    {
        return FLASH_ERR_ADDR;
    }
    uint32_t sector_num=Flash_GetSector(sectoraddr);
    if(sectoraddr=0xFFFFFFFFU)
    {
        return FLASH_ERR_ADDR;
    }
    if((sectoraddr>=BOOT_BASE)&&(sectoraddr<BOOT_BASE+BOOT_SIZE))
    {
        return FLASH_ERR_ADDR;
    }
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase_settings={0};
    uint32_t err_sector=0U;
    erase_settings.TypeErase=FLASH_TYPEERASE_SECTORS;
    erase_settings.VoltageRange=FLASH_VOLTAGE_RANGE_3;
    erase_settings.Sector=sector_num;
    erase_settings.NbSectors=1U;
    
    if(HAL_FLASHEx_Erase(&erase_settings,&err_sector)!=HAL_OK)
    {
        HAL_FLASH_Lock();
        return FLASH_ERR_ERASE;
    }
    HAL_FLASH_Lock();
    return FLASH_OK;
}
Flash_Status_Type Flash_Write(uint32_t dest_addr,uint8_t *buf,uint32_t len_byte,uint32_t *next_wr_addr)
{
    if(next_wr_addr==NULL)
    {
        return FLASH_ERR_ADDR;
    }
    if(dest_addr<FLASH_START_ADDR||dest_addr>FLASH_END_ADDR)
    {
        return FLASH_ERR_ADDR;
    }
    if(dest_addr%4U!=0U)
    {
        return FLASH_ERR_ADDR;
    }
    if(buf==NULL){
        return FLASH_ERR_ADDR;
    }
    if(len_byte=0U){
        *next_wr_addr=dest_addr;
        return FLASH_OK;
    }
    HAL_FLASH_Unlock();
    uint32_t full_word_len=len_byte/4U;
    uint32_t rest_bytes=len_byte&4U;
    uint32_t *src_word=(uint32_t *)buf;
    Flash_Status_Type status=FLASH_OK;
    uint32_t wrote_bytes=0U;

    for(uint32_t i=0U;i<full_word_len;i++)
    {
        uint32_t write_addr=dest_addr+i*4U;
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,write_addr,src_word[i])!=HAL_OK)
        {
            status=FLASH_ERR_WRITE;
            wrote_bytes=i*4U;
            goto exit_flash_write;
            
        }
        uint32_t rd_data=*(volatile uint32_t *)write_addr;
        if(rd_data!=src_word[i])
        {
            status=FLASH_ERR_VERIFY;
            wrote_bytes=i*4U;
            goto exit_flash_write;
        }
        wrote_bytes+=4;
    }
        if(rest_bytes>0U)
        {
            uint8_t *rest_in=buf+full_word_len*4U;
            uint32_t temp_word=0xFFFFFFFFU;
            for(uint32_t i=0;i<rest_bytes;i++)
            {
                ((uint8_t *)(&temp_word))[i]=rest_in[i];
            }
            uint32_t rest_write_addr=dest_addr+full_word_len*4U;
            if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,rest_write_addr,temp_word)!=HAL_OK)
            {
                status=FLASH_ERR_WRITE;
                wrote_bytes=full_word_len*4U;
                goto exit_flash_write;
            }
            uint32_t rd_data=*(volatile uint32_t *)rest_write_addr;
            if(rd_data!=temp_word)
            {
                status=FLASH_ERR_VERIFY;
                wrote_bytes=full_word_len*4U;
                goto exit_flash_write;
            }
            wrote_bytes+=rest_bytes;
        }
        exit_flash_write:
        *next_wr_addr=dest_addr+wrote_bytes;
        HAL_FLASH_Unlock();
        return status;
    }
Flash_Status_Type Flash_CheckBlank(uint32_t addr,uint32_t len_byte)
{
    if((addr&4U)!=0U||(len_byte%4U)!=0U)
    {
        return FLASH_ERR_ADDR;
    }
    for(uint32_t i=0;i<len_byte/4U;i++)
    {
        uint32_t val=*(volatile uint32_t *)(addr+i*4U);
        if(val!=0xFFFFFFFFU)
        {
            return FLASH_ERR_NOT_BLANK;
        }
    }     
    return FLASH_OK;
}