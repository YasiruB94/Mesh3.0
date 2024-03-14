#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CNGW_DIRECT_CONTROL_H
#define CNGW_DIRECT_CONTROL_H

#include "cngw_common.h"


typedef enum CNGW_Direct_Control_Command
{
    CNGW_DIRECT_CONTROL_CMD_Invalid    = 0x00,
    CNGW_DIRECT_CONTROL_CMD_Response,
    CNGW_DIRECT_CONTROL_CMD_Response_Success,
    CNGW_DIRECT_CONTROL_CMD_Response_Fail,
    CNGW_DIRECT_CONTROL_CMD_Mainboard_Restart,
    CNGW_DIRECT_CONTROL_CMD_Read_Flash,
    CNGW_DIRECT_CONTROL_CMD_Driver_Change_Current,

} __attribute__((packed)) CNGW_Direct_Control_Command;

typedef enum CNGW_Direct_Control_Status
{
    CNGW_DIRECT_CONTROL_STATUS_Error        = 0,
    CNGW_DIRECT_CONTROL_STATUS_Success      = 1,

} __attribute__((packed)) CNGW_Direct_Control_Status;

typedef struct CNGW_Direct_Control_Message_t
{
    CNGW_Direct_Control_Command         command;
    CNGW_Source_MCU                     target_MCU;
    uint8_t                             uint8_value;
    uint16_t                            uint16_value;
    uint32_t                            int_value;
    CNGW_Direct_Control_Status          result;
    uint8_t                             crc;
} __attribute__((packed)) CNGW_Direct_Control_Message_t;

typedef struct CNGW_Direct_Control_Frame_t
{
    CNGW_Message_Header_t           header;
    CNGW_Direct_Control_Message_t  message;
} __attribute__((packed)) CNGW_Direct_Control_Frame_t;

#endif
#endif