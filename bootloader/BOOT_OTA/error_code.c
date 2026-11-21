#include "error_code.h"

static const char *const err_str[ERR_NUM]=
{
    "OK",
    "CRC ERROR",
    "LENGTH ERROR",
    "FLASH ERROR",
    "TIMEOUT",
    "EXCEED SLOT SIZE"
};

const char* err_to_str(ota_err_te err)
{
    if(err<ERR_OK||err>=ERR_NUM)return "UNKNOWN ERR";
    return err_str[err];
}
