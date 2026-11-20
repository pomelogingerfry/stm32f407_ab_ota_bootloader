#ifndef __ERROR_CODE_H
#define __ERROR_CODE_H

typedef enum
{
    ERR_OK=0,
    ERR_CRC=1,
    ERR_LEN=2,
    ERR_FLASH=3,
    ERR_TIMEOUT=4,
    ERR_SLOT=5
}ota_err_te;

const char* err_to_str(ota_err_te err);

#endif