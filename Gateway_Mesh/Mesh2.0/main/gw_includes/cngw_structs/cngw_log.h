#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_LOG_H
#define CNGW_LOG_H

#include "cngw_common.h"

typedef enum CNGW_Log_Command
{
    CNGW_LOG_TYPE_INVALID = 0,
    CNGW_LOG_TYPE_ERRCODE = 1,
    CNGW_LOG_TYPE_STRING = 2,
} __attribute__((packed)) CNGW_Log_Command;

typedef enum CNGW_Log_Level
{
    CNGW_LOG_DISABLE = 0,
    CNGW_LOG_ERROR = 1, /*Default for all MCUs*/
    CNGW_LOG_WARNING = 2,
    CNGW_LOG_INFO = 3,
    CNGW_LOG_DEBUG = 4,
    CNGW_LOG_VERBOSE = 5
} __attribute__((packed)) CNGW_Log_Level;

typedef struct CNGW_Log_Message_t
{
    CNGW_Log_Command command;
    uint8_t serial[CNGW_SERIAL_NUMBER_LENGTH]; /*Originating serial # for the log message*/
    CNGW_Log_Level severity;

    /** @note
     * A NULL terminated string with the CRC of
     * this message after the NULL terminator.
     * A string cannot exceed 128 bytes.
     * The string must be NULL terminated so the decoder
     * knows where the string ends.
     *
     * The CRC must always be append directly after the NULL terminator.
     * If a string is 16 bytes then copy 16 bytes, add the NULL terminator in
     * byte 17, then add CRC in byte 18.
     *
     * CRC is computed from the first byte of this struct to string NULL terminator
     * (inclusive of the NULL terminator).
     * That is form \p message_type to \p string_and_crc[NULL_terminator_index]
     * */
    char string_and_crc[128 + 1];
} __attribute__((packed)) CNGW_Log_Message_t;

typedef struct CNGW_Log_Message_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Log_Message_t message;
} __attribute__((packed)) CNGW_Log_Message_Frame_t;

#endif
#endif