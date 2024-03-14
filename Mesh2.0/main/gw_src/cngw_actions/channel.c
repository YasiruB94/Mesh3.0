/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: channel.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending channel commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "gw_includes/channel.h"
#include "gw_includes/ccp_util.h"
static const char *TAG = "channel";

esp_err_t send_channel_message(const CNGW_Config_Status msg_status)
{
    
    CCP_TX_FRAMES_t lmcontrol_res;
    CNGW_Channel_Status_Frame_t channel_status_Frame;
    CCP_UTIL_Get_Msg_Header(&channel_status_Frame.header, CNGW_HEADER_TYPE_Configuration_Command, sizeof(channel_status_Frame.message));
    channel_status_Frame.message.command = CNGW_CONFIG_CMD_Channel_Status;
    channel_status_Frame.message.status = msg_status;
    channel_status_Frame.message.u.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(channel_status_Frame.message), sizeof(channel_status_Frame.message) - sizeof(channel_status_Frame.message.u.crc));
     ESP_LOGI(TAG, "message.command: %d", channel_status_Frame.message.command);
     ESP_LOGI(TAG, "message.status: %d", channel_status_Frame.message.status);
     ESP_LOGI(TAG, "message.u.crc: %d", channel_status_Frame.message.u.crc);

    lmcontrol_res.channel_status = &channel_status_Frame;
    consume_GW_message((uint8_t *)&lmcontrol_res);
    
    return ESP_OK;
}
#endif