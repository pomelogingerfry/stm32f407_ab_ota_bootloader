#include "error_code.h"
const char* err_to_str(ota_err_te err)
{
    switch(err)
    {
        case ERR_OK:return "OK";
        case ERR_CRC:return "CRC ERROR";
        case ERR_LEN:return "LENGTH ERROR";
        case ERR_FLASH:return "FLASH ERROR";
        case ERR_TIMEOUT:return "TIMEOUT";
        case ERR_SLOT:return "EXCEED SLOT SIZE";
        default:return "UNKONWN ERR";
    }
}