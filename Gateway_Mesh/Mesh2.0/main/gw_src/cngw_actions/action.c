/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: action.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: legacy functions used in sending action commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/action.h"
// static const char *TAG = "action";
bool print_send_data = true;

// Legacy codes: not being used now
esp_err_t Send_Action_Msg_On_Off(uint16_t address, CNGW_Action_Command cmd, uint16_t level)
{
    CCP_TX_FRAMES_t lmcontrol_res = {0};
    CNGW_Action_Frame_t actionFrame = {0};
    CCP_UTIL_Get_Msg_Header(&actionFrame.header, CNGW_HEADER_TYPE_Action_Commmand, sizeof(actionFrame.message));
    actionFrame.message.command = cmd;
    actionFrame.message.address.target_cabinet = cn_board_info.cn_config.cabinet_number;
    actionFrame.message.address.address_type = CNGW_ADDRESS_TYPE_Cense_Channel;
    actionFrame.message.address.target_address = address;
    actionFrame.message.action_parameters.fade_unit = CNGW_FADE_UNIT_Milliseconds;
    actionFrame.message.action_parameters.fade_time = 0;
    actionFrame.message.action_parameters.light_level = level;
    actionFrame.message.action_parameters.reserved = 0;
    actionFrame.message.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(actionFrame.message), sizeof(actionFrame.message) - sizeof(actionFrame.message.crc));
    lmcontrol_res.action = &actionFrame;
    if (print_send_data)
    {
        unsigned char byteArray[sizeof(CNGW_Action_Frame_t)];
        memcpy(byteArray, &actionFrame, sizeof(CNGW_Action_Frame_t));
        printf("Send_Action_Msg_On_Off: ");
        int i = 0;
        while (i < sizeof(CNGW_Action_Frame_t))
        {
            printf("%02X ",byteArray[i]);
            i++;
        }
        printf("\n");
    }
    consume_GW_message((uint8_t *)&lmcontrol_res);
    return ESP_OK;
}
esp_err_t Send_Action_Msg_fade(uint16_t address, CNGW_Action_Command cmd, uint16_t level, CNGW_Fade_Unit fade_unit, uint16_t fade_time)
{
    CCP_TX_FRAMES_t lmcontrol_res = {0};
    CNGW_Action_Frame_t actionFrame = {0};
    CCP_UTIL_Get_Msg_Header(&actionFrame.header, CNGW_HEADER_TYPE_Action_Commmand, sizeof(actionFrame.message));
    actionFrame.message.command = cmd;
    actionFrame.message.address.target_cabinet = cn_board_info.cn_config.cabinet_number;
    actionFrame.message.address.address_type = CNGW_ADDRESS_TYPE_Cense_Channel;
    actionFrame.message.address.target_address = address;
    actionFrame.message.action_parameters.fade_unit = fade_unit;
    actionFrame.message.action_parameters.fade_time = fade_time;
    actionFrame.message.action_parameters.light_level = level;
    actionFrame.message.action_parameters.reserved = 0;
    actionFrame.message.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(actionFrame.message), sizeof(actionFrame.message) - sizeof(actionFrame.message.crc));
    lmcontrol_res.action = &actionFrame;
    if (print_send_data)
    {
        unsigned char byteArray[sizeof(CNGW_Action_Frame_t)];
        memcpy(byteArray, &actionFrame, sizeof(CNGW_Action_Frame_t));
        printf("Send_Action_Msg_fade: ");
        int i = 0;
        while (i < sizeof(CNGW_Action_Frame_t))
        {
            printf("%02X ",byteArray[i]);
            i++;
        }
        printf("\n");
    }
    consume_GW_message((uint8_t *)&lmcontrol_res);
    return ESP_OK;
}

esp_err_t Send_Action_Msg_broadcast(CNGW_Action_Command cmd, uint16_t level, CNGW_Fade_Unit fade_unit, uint16_t fade_time)
{
    CCP_TX_FRAMES_t lmcontrol_res = {0};
    CNGW_Action_Frame_t actionFrame = {0};
    CCP_UTIL_Get_Msg_Header(&actionFrame.header, CNGW_HEADER_TYPE_Action_Commmand, sizeof(actionFrame.message));
    actionFrame.message.command = cmd;
    actionFrame.message.address.target_cabinet = cn_board_info.cn_config.cabinet_number;
    actionFrame.message.address.address_type = CNGW_ADDRESS_TYPE_Cense_Broadcast;
    actionFrame.message.action_parameters.fade_unit = fade_unit;
    actionFrame.message.action_parameters.fade_time = fade_time;
    actionFrame.message.action_parameters.light_level = level;
    actionFrame.message.action_parameters.reserved = 0;
    actionFrame.message.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(actionFrame.message), sizeof(actionFrame.message) - sizeof(actionFrame.message.crc));
    lmcontrol_res.action = &actionFrame;
    if (print_send_data)
    {
        unsigned char byteArray[sizeof(CNGW_Action_Frame_t)];
        memcpy(byteArray, &actionFrame, sizeof(CNGW_Action_Frame_t));
        printf("Send_Action_Msg_broadcast: ");
        int i = 0;
        while (i < sizeof(CNGW_Action_Frame_t))
        {
            printf("%02X ",byteArray[i]);
            i++;
        }
        printf("\n");
    }
    consume_GW_message((uint8_t *)&lmcontrol_res);
    return ESP_OK;
}

#endif