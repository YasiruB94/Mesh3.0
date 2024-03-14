#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_CONTROL_H
#define CNGW_CONTROL_H

#include "cngw_common.h"
#include "cngw_log.h"


typedef enum CNGW_Control_Command
{
    CNGW_CONTROL_CMD_Invalid = 0x00,
    CNGW_CONTROL_CMD_Log
} __attribute__((packed)) CNGW_Control_Command;

typedef struct CNGW_Control_Message_t
{
    CNGW_Control_Command command;
    CNGW_Log_Level level;
    CNGW_Source_MCU mcu_id;
    uint8_t crc;
} __attribute__((packed)) CNGW_Control_Message_t;

typedef struct CNGW_Control_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Control_Message_t message;
} __attribute__((packed)) CNGW_Control_Frame_t;

#endif
#endif