/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: direct_control.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending direct control commands to mainboard
 * NOTE: New addition to the cence FW > 2.5.3.0. used in sending new information like
 * updating the current for drivers, etc
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/direct_control.h"
#include "gw_includes/ccp_util.h"

esp_err_t send_direct_control_message(CNGW_Direct_Control_Command cmd, CNGW_Source_MCU target_MCU, uint8_t uint8_value, uint16_t uint16_value, uint32_t int_value)
{
    CCP_TX_FRAMES_t lmcontrol_res;
    CNGW_Direct_Control_Frame_t directControlFrame;
    CCP_UTIL_Get_Msg_Header(&directControlFrame.header, CNGW_HEADER_TYPE_Direct_Control_Command, sizeof(directControlFrame.message));
    directControlFrame.message.command = cmd;
    directControlFrame.message.target_MCU = target_MCU;
    directControlFrame.message.uint8_value = uint8_value;
    directControlFrame.message.uint16_value = uint16_value;
    directControlFrame.message.int_value = int_value;
    directControlFrame.message.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(directControlFrame.message), sizeof(directControlFrame.message) - sizeof(directControlFrame.message.crc));
    lmcontrol_res.direct_control_commands = &directControlFrame;
    consume_GW_message((uint8_t *)&lmcontrol_res);
    
    return ESP_OK;
}
#endif