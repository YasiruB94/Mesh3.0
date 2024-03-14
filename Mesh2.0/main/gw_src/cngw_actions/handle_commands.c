/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: handle_commands.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in decoding the recieving commands/ messages from
 * mainboard and passing the decoded infomation to the approptiate functions
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "gw_includes/handle_commands.h"
#include "gw_includes/logging.h"
#include "gw_includes/ota_agent.h"
#include "gw_includes/ccp_util.h"
static const char *TAG = "handle_commands";

#ifdef GW_DEBUGGING
static bool ignore_crc_check        = false;
static bool print_all_frame_info    = false;
static bool print_header_frame_info = true;
#else
static bool ignore_crc_check        = false;
static bool print_all_frame_info    = false;
static bool print_header_frame_info = false;
#endif

void printBits(uint32_t num)
{
    for (int i = 31; i >= 0; i--)
    {
        uint32_t mask = 1u << i;
        printf("%u", (num & mask) ? 1 : 0);

        if (i % 4 == 0)
            printf(" "); // Print a space every 4 bits for better readability
    }
    printf("\n");
}

inline uint8_t Is_Header_Valid(const CNGW_Message_Header_t *const header)
{
    /*Calculate CRC and compare*/
    const size_t crc_size = sizeof(CNGW_Message_Header_t) - sizeof(header->crc);
    const uint8_t crc8 = CCP_UTIL_Get_Crc8(0, (uint8_t *)header, crc_size);

    return (crc8 == header->crc);
}

size_t Parse_1_Frame(uint8_t *packet, size_t dataSize)
{
    CNGW_Message_Header_t header = {0};
    if (dataSize >= sizeof(CNGW_Message_Header_t))
    {   size_t next_buffer_pos = 1;
        memcpy(&header, packet, sizeof(CNGW_Message_Header_t));
        if (!Is_Header_Valid(&header))
        {
            //didnt find a valid header. increment the packet read by one.
            return next_buffer_pos;
        }
        switch (header.command_type)
        {
        case CNGW_HEADER_TYPE_Query_Command:
        {
            CNGW_Query_Message_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));
            if (frame.message.command == CNGW_QUERY_CMD_Backward_Frame)
            {
                const size_t message_crc_size = sizeof(frame.message) - sizeof(frame.message.crc);
                const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame.message, message_crc_size);
                if (message_crc8 == frame.message.crc || ignore_crc_check)
                {
                    next_buffer_pos = sizeof(frame);
                    if (print_header_frame_info || print_all_frame_info)
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Query_Command: CNGW_QUERY_CMD_Backward_Frame. bd_query_received");
                    }
                }
            }
            else if (frame.message.command == CNGW_QUERY_CMD_Get_All_Channel_Info)
            {
                const size_t message_crc_size = sizeof(frame.message) - sizeof(frame.message.crc);
                const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame.message, message_crc_size);
                if (message_crc8 == frame.message.crc || ignore_crc_check)
                {
                    next_buffer_pos = sizeof(frame);
                    if (print_header_frame_info || print_all_frame_info)
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Query_Command: CNGW_QUERY_CMD_Get_All_Channel_Info.");
                    }
                }
            }
        }
        break;
        case CNGW_HEADER_TYPE_Status_Update_Command:
        {
            CNGW_Update_Channel_Status_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));
            if (frame.message.command_type == CNGW_UPDATE_CMD_Status)
            { // status
                const size_t message_crc_size = sizeof(frame.message) - sizeof(frame.message.crc);
                const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame.message, message_crc_size);
                if (message_crc8 == frame.message.crc || ignore_crc_check)
                {
                    next_buffer_pos = sizeof(frame);
                    if (print_header_frame_info || print_all_frame_info)
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Status_Update_Command STATUS");
                    }
                    if (print_all_frame_info)
                    {

                        ESP_LOGI(TAG, "command_type: %d", frame.message.command_type);
                        ESP_LOGI(TAG, "address.target_cabinet: %d", frame.message.address.target_cabinet);
                        ESP_LOGI(TAG, "address.address_type: %d", frame.message.address.address_type);
                        ESP_LOGI(TAG, "address.target_address: %d", frame.message.address.target_address);
                        ESP_LOGI(TAG, "status_mask: %u", frame.message.status_mask); // bitwise
                        printBits(frame.message.status_mask);
                    }
                    //  save information to the cn_board_info
                    cn_board_info.channel_status[frame.message.address.target_address] = frame.message;
                }
            }
            else if (frame.message.command_type == CNGW_UPDATE_CMD_Attribute)
            { // attribute
                CNGW_Update_Attribute_Frame_t frame2 = {0};
                memcpy(&frame2, packet, sizeof(frame2));
                const size_t message_crc_size = sizeof(frame2.message) - sizeof(frame2.message.crc);
                const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame2.message, message_crc_size);
                if (message_crc8 == frame2.message.crc || ignore_crc_check)
                {
                    next_buffer_pos = sizeof(frame2);
                    if (print_header_frame_info || print_all_frame_info)
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Status_Update_Command ATTRIBUTE");
                    }
                    if (print_all_frame_info)
                    {

                        ESP_LOGI(TAG, "command_type: %d", frame2.message.command_type);
                        ESP_LOGI(TAG, "address.target_cabinet: %d", frame2.message.address.target_cabinet);
                        ESP_LOGI(TAG, "address.address_type: %d", frame2.message.address.address_type);
                        ESP_LOGI(TAG, "address.target_address: %d", frame2.message.address.target_address);
                        ESP_LOGI(TAG, "attribute: %d", frame2.message.attribute);
                        ESP_LOGI(TAG, "value: %d", frame2.message.value);
                    }
                    //  save informwation to the cn_board_info:
                    cn_board_info.channel_attribute[frame2.message.address.target_address] = frame2.message;
                }
            }
        }
        break;
        case CNGW_HEADER_TYPE_Ota_Command:
        {
            next_buffer_pos = sizeof(CNGW_Ota_Status_Frame_t);
            CNGW_Ota_Status_Frame_t frame2 = {0};
            memcpy(&frame2, packet, sizeof(frame2));

            if (frame2.message.status == CNGW_OTA_STATUS_Restart)
            {
                ESP_LOGE(TAG, "OTA RESTART REQUIRED");
                OTA_Restart_Required();
            }
            else if (frame2.message.status == CNGW_OTA_STATUS_Success)
            {
                ESP_LOGW(TAG, "CN CONFIRMED PROPER OTA");
                OTA_FW_Success();
            }
            else if (frame2.message.status == CNGW_OTA_STATUS_Ack)
            {
                OTA_next_Frame(true);
            }
            else
            {
                // not checking crc
                OTA_next_Frame(false);
            }
        }
        break;
        case CNGW_HEADER_TYPE_Log_Command:
        {
            CNGW_Log_Message_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));
            next_buffer_pos = sizeof(frame);
            if (frame.message.command == CNGW_LOG_TYPE_ERRCODE)
            {
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_LOG_TYPE_ERRCODE");
                }
                Handle_Error_Message(packet);
            }
            else if (frame.message.command == CNGW_LOG_TYPE_STRING)
            {
                // not checking crc
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_LOG_TYPE_STRING");
                }
                Handle_Log_Message(packet);
            }
        }
        break;
        case CNGW_HEADER_TYPE_Config_Message_Command:
        {
            CNGW_Config_Message_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));
            const size_t message_crc_size = sizeof(frame.message) - sizeof(frame.message.crc);
            const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame.message, message_crc_size);
            if (message_crc8 == frame.message.crc || ignore_crc_check)
            {
                next_buffer_pos = sizeof(frame);
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Config_Message_Command, message.command: CNGW_CONFIG_CMD_Channel_Status");
                }
                Handle_Configuration_Information_Message(packet);
            }
        }
        break;
        case CNGW_HEADER_TYPE_Configuration_Command:
        {
            CNGW_Channel_Status_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));
            next_buffer_pos = sizeof(frame);
            if (frame.message.command == CNGW_CONFIG_CMD_Invalid)
            {
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Configuration_Command, config type: CNGW_CONFIG_CMD_Invalid");
                }
            }
            else if (frame.message.command == CNGW_CONFIG_CMD_Channel_Entry)
            {
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Configuration_Command, config type: CNGW_CONFIG_CMD_Channel_Entry");
                }
            }
            else if (frame.message.command == CNGW_CONFIG_CMD_Channel_Status)
            {
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Configuration_Command, config type: CNGW_CONFIG_CMD_Channel_Status");
                }
                Handle_Channel_Status_Message(packet);
            }
        }
        break;
        case CNGW_HEADER_TYPE_Device_Report:
        {
            CNGW_Device_Info_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));

            if (frame.message.command == CNGW_DEVINFO_CMD_Update)
            {
                // Info about a new device being added to the mainboard
                const size_t message_crc_size = sizeof(frame.message) - sizeof(frame.message.crc);
                const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame.message, message_crc_size);
                if (message_crc8 == frame.message.crc || ignore_crc_check)
                {
                    next_buffer_pos = sizeof(frame);
                    if (print_header_frame_info || print_all_frame_info)
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Device_Report CNGW_DEVINFO_CMD_Update");
                    }
                    if (print_all_frame_info)
                    {
                        ESP_LOGI(TAG, "frame.message.mcu: %d", frame.message.mcu);
                        ESP_LOGI(TAG, "frame.message.serial: %02X%02X%02X%02X%02X%02X%02X%02X%02X", frame.message.serial[0], frame.message.serial[1], frame.message.serial[2], frame.message.serial[3], frame.message.serial[4], frame.message.serial[5], frame.message.serial[6], frame.message.serial[7], frame.message.serial[8]);
                        ESP_LOGI(TAG, "frame.message.model: %u", frame.message.model);
                        ESP_LOGI(TAG, "frame.message.hardware_version: %u.%u-%u", frame.message.hardware_version.major, frame.message.hardware_version.minor, frame.message.hardware_version.ci);
                        ESP_LOGI(TAG, "frame.message.bootloader_version: %u.%u.%u-%u", frame.message.bootloader_version.major, frame.message.bootloader_version.minor, frame.message.bootloader_version.ci, frame.message.bootloader_version.branch_id);
                        ESP_LOGI(TAG, "frame.message.application_version: %u.%u.%u-%u", frame.message.application_version.major, frame.message.application_version.minor, frame.message.application_version.ci, frame.message.application_version.branch_id);
                    }

                    // save informwation to the cn_board_info:
                    switch (frame.message.mcu)
                    {
                    case 0:
                        // it is the cn mcu
                        cn_board_info.cn_mcu = frame.message;
                        cn_board_info.cn_config.cn_is_handshake = true;
                        xQueueReset(GW_response_queue);
                        // change the LED appearance
                        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
                        // delete the timer associated with checking the GW availability periodically
                        ESP_LOGI(TAG, "HANDSHAKE DONE!");
                        xTimerStop(GW_Availability_Timer  , portMAX_DELAY);
                        xTimerDelete(GW_Availability_Timer, portMAX_DELAY);
                        GW_Availability_Timer             = NULL;

                        Send_GW_message_to_AWS(64, 0, "CONNECTED TO CENCE!\0");
                        break;
                    case 1:
                        // it is the sw mcu
                        cn_board_info.sw_mcu = frame.message;
                        break;
                    case 2:
                        // value not handled
                        break;
                    default:
                        // it is a dr mcu
                        cn_board_info.dr_mcu[frame.message.mcu - 3] = frame.message;
                        break;
                    }
                }
            }
            else if (frame.message.command == CNGW_DEVINFO_CMD_Remove)
            {
                // Info about a device being removed from the main board.
                CNGW_Device_Info_Remove_Frame_t frame_mod = {0};
                memcpy(&frame_mod, packet, sizeof(frame_mod));
                next_buffer_pos = sizeof(frame_mod);
                if (print_header_frame_info || print_all_frame_info)
                {
                    ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Device_Report CNGW_DEVINFO_CMD_Remove");
                }
                if (print_all_frame_info)
                {
                    ESP_LOGI(TAG, "frame.message.mcu: %d", frame_mod.message.mcu);
                }
                // save informwation to the cn_board_info:
                switch (frame_mod.message.mcu)
                {
                case 0:
                    // it is the cn mcu
                    cn_board_info.cn_mcu.command = frame_mod.message.command;
                    break;
                case 1:
                    // it is the sw mcu
                    cn_board_info.sw_mcu.command = frame_mod.message.command;
                    break;
                default:
                    // it is a dr mcu
                    cn_board_info.dr_mcu[frame_mod.message.mcu - 2].command = frame_mod.message.command;
                    break;
                }
            }
        }
        break;
        case CNGW_HEADER_TYPE_Handshake_Command:
        {
            // if a handshake message comes in the middle of an OTA FW update, ignore the message
            if (!ota_agent_core_OTA_in_progress)
            {
                CNGW_Handshake_CN1_Frame_t frame = {0};
                memcpy(&frame, packet, sizeof(frame));
                switch (frame.message.command)
                {
                case CNGW_Handshake_CMD_CN1:
                {
                    LED_assign_task(CNGW_LED_CMD_CONN_STAGE_01, CNGW_LED_CN);
                    analyze_CNGW_Handshake_CN1_t(packet, BUFFER);
                    next_buffer_pos = sizeof(frame);
                }
                break;
                case CNGW_Handshake_CMD_CN2:
                {
                    LED_assign_task(CNGW_LED_CMD_CONN_STAGE_02, CNGW_LED_CN);
                    analyze_CNGW_Handshake_CN2_t(packet, BUFFER);
                    next_buffer_pos = sizeof(frame);
                }
                break;
                default:
                    break;
                }
            }
        }
        break;
        case CNGW_HEADER_TYPE_Direct_Control_Command:
        {
            // even though i get the message here, i can't still decode the frame.message.result
            CNGW_Direct_Control_Frame_t frame = {0};
            memcpy(&frame, packet, sizeof(frame));
            const size_t message_crc_size = sizeof(frame.message) - sizeof(frame.message.crc);
            const uint8_t message_crc8 = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&frame.message, message_crc_size);
            
            if (message_crc8 == frame.message.crc || true)
            {
                next_buffer_pos = sizeof(frame);
                if (print_header_frame_info || print_all_frame_info || true)
                {
                    ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Direct_Control_Command,");
                }

                switch(frame.message.result)
                {
                    case CNGW_DIRECT_CONTROL_STATUS_Success:
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Direct_Control_Command is a response message SUCCESS!");
                    }
                    break;
                    case CNGW_DIRECT_CONTROL_STATUS_Error:
                    {
                        ESP_LOGI(TAG, "message type: CNGW_HEADER_TYPE_Direct_Control_Command is a response message FAIL!");
                    }
                    break;
                    default:
                    {

                    }
                    break;
                }
            }
        }
        break;
        default:
        {
        }
        break;
        }

        // return the next expected buffer position to be read from
        return next_buffer_pos;
    }
    else
    {
        //the data is smaller than the header. Return the size of incoming buffer so we exit the iteration
        return dataSize;

    }
}
#endif