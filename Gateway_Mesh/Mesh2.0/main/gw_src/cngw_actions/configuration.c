/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: configuration.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending configuration commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/configuration.h"
static const char *TAG = "configuration";

#ifdef GATEWAY_DEBUGGING
static bool print_all_incoming_information  = false;
#else
static bool print_all_incoming_information  = false;
#endif

TaskHandle_t ConfigTask;
static bool exit_task = false;
static bool resume_task = false;
static uint8_t next_expected_command = 0;
static uint8_t next_expected_slot = 0;
static uint16_t timeout_ticks = 0;

void Handle_Channel_Status_Message(const uint8_t *recvbuf)
{
    CNGW_Channel_Status_Frame_t frame = {0};
    memcpy(&frame, recvbuf, sizeof(frame));

    // Channel set status message
    switch (frame.message.status)
    {
    case CNGW_CONFIG_STATUS_Invalid:
        ESP_LOGW(TAG, "message status: CNGW_CONFIG_STATUS_Invalid");
        break;
    case CNGW_CONFIG_STATUS_Done:
        ESP_LOGW(TAG, "message status: CNGW_CONFIG_STATUS_Done");
        Request_Configuration_information();
        break;
    case CNGW_CONFIG_STATUS_Get_Next_Msg:
        ESP_LOGW(TAG, "message status: CNGW_CONFIG_STATUS_Get_Next_Msg");
        break;
    case CNGW_CONFIG_STATUS_Restart:
        ESP_LOGW(TAG, "message status: CNGW_CONFIG_STATUS_Restart. Restarting the ESP...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
        break;
    case CNGW_CONFIG_STATUS_Config_Change:
        ESP_LOGW(TAG, "message status: CNGW_CONFIG_STATUS_Config_Change");
        CNGW_Channel_Status_Frame_t Frame;
        CCP_UTIL_Get_Msg_Header(&Frame.header, CNGW_HEADER_TYPE_Configuration_Command, sizeof(Frame.message));
        Frame.message.command = CNGW_CONFIG_CMD_Channel_Status;
        Frame.message.status = CNGW_CONFIG_STATUS_Get_Next_Msg;
        Frame.message.u.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(Frame.message), sizeof(Frame.message) - sizeof(Frame.message.u));
        consume_GW_message((uint8_t *)&Frame);
        break;

    default:
        break;
    }
}

void Request_Configuration_information()
{
    ESP_LOGI(TAG, "Request_Configuration_information from mainboard ...");
    if (ConfigTask == NULL)
    {
        xTaskCreate(configuration_request_loop, "configuration_request_loop", 2047, NULL, 10, &ConfigTask);
    }
    else
    {
        ESP_LOGE(TAG, "The task is already running!");
        Send_GW_message_to_AWS(65, 0, "The task is already running!");
    }
}

void configuration_request_loop(void *pvParameters)
{
    exit_task = false;
    resume_task = true;
    next_expected_slot = 0;
    next_expected_command = 0;
    next_expected_command = CNGW_CONFIG_CMD_Config_General_Info;
    TickType_t beginningTick = xTaskGetTickCount();
    TickType_t currentTick = xTaskGetTickCount();
    TickType_t lastRequestTick = currentTick;

    if(print_all_incoming_information)
    {
        timeout_ticks = 7800;
    }
    else
    {
        timeout_ticks = 1800;
    }

    LED_assign_task(CNGW_LED_CMD_GET_CONFIG_INFO, CNGW_LED_CN);

    while (1)
    {
        currentTick = xTaskGetTickCount();
        if (currentTick - beginningTick > timeout_ticks)
        {
            ESP_LOGE(TAG, "configuration request loop timeout");
            Send_GW_message_to_AWS(65, 0, "configuration request loop timeout");
            LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
            LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_CN, LED_CHANGE_EXTENDED_DURATION);
            exit_task = true;
        }
        if (currentTick - lastRequestTick > 100)
        {
            ESP_LOGW(TAG, "Resending previous message...");
            resume_task = true;
        }

        if (resume_task && !exit_task)
        {
            CNGW_Config_Request_Frame_t Frame;
            CCP_UTIL_Get_Msg_Header(&Frame.header, CNGW_HEADER_TYPE_Configuration_Request_Command, sizeof(Frame.message));
            Frame.message.command = next_expected_command;
            Frame.message.slot = next_expected_slot;
            Frame.message.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(Frame.message), sizeof(Frame.message) - sizeof(Frame.message.crc));
            consume_GW_message((uint8_t *)&Frame);
            resume_task = false;
            lastRequestTick = currentTick;
        }

        if (exit_task)
        {
            ESP_LOGI(TAG, "Exiting the configuration request loop");
            LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
            ConfigTask = NULL;
            vTaskDelete(ConfigTask);
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void Handle_Configuration_Information_Message(const uint8_t *recvbuf)
{
    if (!exit_task)
    {
        CNGW_Config_Message_Frame_t frame = {0};
        memcpy(&frame, recvbuf, sizeof(frame));
        if(print_all_incoming_information)
        {
            ESP_LOGI(TAG, "Receiving command %d\tReceiving slot %d", frame.message.command, frame.message.slot);
        }

        switch (frame.message.command)
        {
        case CNGW_CONFIG_CMD_Config_General_Info:
        {
            CN_General_Config_Reduced_t temp_buf = {0};
            memcpy(&temp_buf, &frame.message.frame_bytes, sizeof(CN_General_Config_Reduced_t));
            for (uint8_t i = 0; i < 9; i++)
            {
                cn_board_info.cn_config.serial_id[i] = temp_buf.serial_id[i];
            }
            cn_board_info.cn_config.dali_ps_active = temp_buf.dali_ps_active;
            cn_board_info.cn_config.slot_count = temp_buf.slot_count;
            cn_board_info.cn_config.wired_switch_count = temp_buf.wired_switch_count;
            cn_board_info.cn_config.wireless_switch_count = temp_buf.wireless_switch_count;
            cn_board_info.cn_config.cabinet_number = temp_buf.cabinet_number;
            cn_board_info.cn_config.flash_programmed = temp_buf.flash_programmed;
            cn_board_info.cn_config.sw_version = temp_buf.sw_version;
            cn_board_info.cn_config.sw_bootloader_version = temp_buf.sw_bootloader_version;
            cn_board_info.cn_config.sw_is_handshake = temp_buf.sw_is_handshake;

            if (print_all_incoming_information)
            {
                ESP_LOGI(TAG, "cn_board_info.cn_config.serial_id: %02X%02X%02X%02X%02X%02X%02X%02X%02X"   , cn_board_info.cn_config.serial_id[0], cn_board_info.cn_config.serial_id[1], cn_board_info.cn_config.serial_id[2], cn_board_info.cn_config.serial_id[3], cn_board_info.cn_config.serial_id[4], cn_board_info.cn_config.serial_id[5], cn_board_info.cn_config.serial_id[6], cn_board_info.cn_config.serial_id[7], cn_board_info.cn_config.serial_id[8]);
                ESP_LOGI(TAG, "cn_board_info.cn_config.dali_ps_active: %d"                                , cn_board_info.cn_config.dali_ps_active);
                ESP_LOGI(TAG, "cn_board_info.cn_config.slot_count: %d"                                    , cn_board_info.cn_config.slot_count);
                ESP_LOGI(TAG, "cn_board_info.cn_config.wired_switch_count: %d"                            , cn_board_info.cn_config.wired_switch_count);
                ESP_LOGI(TAG, "cn_board_info.cn_config.wireless_switch_count: %d"                         , cn_board_info.cn_config.wireless_switch_count);
                ESP_LOGI(TAG, "cn_board_info.cn_config.cabinet_number: %d"                                , cn_board_info.cn_config.cabinet_number);
                ESP_LOGI(TAG, "cn_board_info.cn_config.flash_programmed: %d"                              , cn_board_info.cn_config.flash_programmed);
                ESP_LOGI(TAG, "cn_board_info.cn_config.sw_version: %d.%d.%d-%d"                           , cn_board_info.cn_config.sw_version.major, cn_board_info.cn_config.sw_version.minor, cn_board_info.cn_config.sw_version.ci, cn_board_info.cn_config.sw_version.branch_id);
                ESP_LOGI(TAG, "cn_board_info.cn_config.sw_bootloader_version: %d.%d.%d-%d"                , cn_board_info.cn_config.sw_bootloader_version.major, cn_board_info.cn_config.sw_bootloader_version.minor, cn_board_info.cn_config.sw_bootloader_version.ci, cn_board_info.cn_config.sw_bootloader_version.branch_id);
                ESP_LOGI(TAG, "cn_board_info.cn_config.sw_is_handshake: %d"                               , cn_board_info.cn_config.sw_is_handshake);
            }
            next_expected_slot = 0;
            next_expected_command = CNGW_CONFIG_CMD_Config_Info_Driver;
        }
        break;

        case CNGW_CONFIG_CMD_Config_Info_Driver:
        {
            // Decide on the actual slot and the partition of data requested
            // (value >= lowerBound && value < upperBound)
            uint8_t true_slot = 0;
            uint8_t slot_partition = 0;
            if (frame.message.slot < 5)
            { // message request is for the first slot
                true_slot = 0;
                slot_partition = frame.message.slot - 0;
            }
            else if (frame.message.slot >= 5 && frame.message.slot < 10)
            { // message request is for the second slot
                true_slot = 1;
                slot_partition = frame.message.slot - 5;
            }
            else if (frame.message.slot >= 10 && frame.message.slot < 15)
            { // message request is for the third slot
                true_slot = 2;
                slot_partition = frame.message.slot - 10;
            }
            else if (frame.message.slot >= 15 && frame.message.slot < 20)
            { // message request is for the fourth slot
                true_slot = 3;
                slot_partition = frame.message.slot - 15;
            }
            else if (frame.message.slot >= 20 && frame.message.slot < 25)
            { // message request is for the fifth slot
                true_slot = 4;
                slot_partition = frame.message.slot - 20;
            }
            else if (frame.message.slot >= 25 && frame.message.slot < 30)
            { // message request is for the sixth slot
                true_slot = 5;
                slot_partition = frame.message.slot - 25;
            }
            else if (frame.message.slot >= 30 && frame.message.slot < 35)
            { // message request is for the seventh slot
                true_slot = 6;
                slot_partition = frame.message.slot - 30;
            }
            else if (frame.message.slot >= 35 && frame.message.slot < 40)
            { // message request is for the eighth slot
                true_slot = 7;
                slot_partition = frame.message.slot - 35;
            }
            else
            {
                // wrong numbers
            }

            // record the required data based on the above values
            switch (slot_partition)
            {
            case 0:
            {
                CN_DRV_Slot_partition_01_t temp_buf = {0};
                memcpy(&temp_buf, &frame.message.frame_bytes, sizeof(CN_DRV_Slot_partition_01_t));

                driver_slots[true_slot].status                         = temp_buf.status;
                driver_slots[true_slot].config_status                  = temp_buf.config_status;
                driver_slots[true_slot].slotNumber                     = temp_buf.slotNumber;
                driver_slots[true_slot].driverType                     = temp_buf.driverType;
                driver_slots[true_slot].driverCount                    = temp_buf.driverCount;
                driver_slots[true_slot].firmware_version               = temp_buf.firmware_version;

                for (uint8_t i = 0; i < 9; i++)
                {
                    driver_slots[true_slot].serial_number[i]           = temp_buf.serial_number[i];
                }

                driver_slots[true_slot].model                          = temp_buf.model;
                driver_slots[true_slot].channel_state_last_update_tick = temp_buf.channel_state_last_update_tick;
                driver_slots[true_slot].bootloader_version             = temp_buf.bootloader_version;
                driver_slots[true_slot].stack_remaining                = temp_buf.stack_remaining;
                driver_slots[true_slot].hardware_version               = temp_buf.hardware_version;
                driver_slots[true_slot].product_id                     = temp_buf.product_id;
                driver_slots[true_slot].logging_level                  = temp_buf.logging_level;
                driver_slots[true_slot].last_reset_reason              = temp_buf.last_reset_reason;
                driver_slots[true_slot].last_frame_received_tick       = temp_buf.last_frame_received_tick;

                if (print_all_incoming_information)
                {
                    ESP_LOGI(TAG, "CN_DRV.status: %d"                           , temp_buf.status);
                    ESP_LOGI(TAG, "CN_DRV.config_status: %d"                    , temp_buf.config_status);
                    ESP_LOGI(TAG, "CN_DRV.slotNumber: %d"                       , temp_buf.slotNumber);
                    ESP_LOGI(TAG, "CN_DRV.driverType: %d"                       , temp_buf.driverType);
                    ESP_LOGI(TAG, "CN_DRV.driverCount: %d"                      , temp_buf.driverCount);
                    ESP_LOGI(TAG, "CN_DRV.firmware_version: %d.%d.%d-%d"        , temp_buf.firmware_version.major_version, temp_buf.firmware_version.minor_version, temp_buf.firmware_version.ci_build_number, temp_buf.firmware_version.branch_id);
                    ESP_LOGI(TAG, "CN_DRV.serial_number: %02X%02X%02X%02X%02X%02X%02X%02X%02X", temp_buf.serial_number[0], temp_buf.serial_number[1], temp_buf.serial_number[2], temp_buf.serial_number[3], temp_buf.serial_number[4], temp_buf.serial_number[5], temp_buf.serial_number[6], temp_buf.serial_number[7], temp_buf.serial_number[8]);
                    ESP_LOGI(TAG, "CN_DRV.model: %d"                            , temp_buf.model);
                    ESP_LOGI(TAG, "CN_DRV.channel_state_last_update_tick: %d"   , temp_buf.channel_state_last_update_tick);
                    ESP_LOGI(TAG, "CN_DRV.bootloader_version: %d.%d.%d-%d"      , temp_buf.bootloader_version.major_version, temp_buf.bootloader_version.minor_version, temp_buf.bootloader_version.ci_build_number, temp_buf.bootloader_version.branch_id);
                    ESP_LOGI(TAG, "CN_DRV.stack_remaining: %d"                  , temp_buf.stack_remaining);
                    ESP_LOGI(TAG, "CN_DRV.hardware_version: %d.%d.%d-%d"        , temp_buf.hardware_version.major_version, temp_buf.hardware_version.minor_version, temp_buf.hardware_version.ci_build_number, temp_buf.hardware_version.branch_id);
                    ESP_LOGI(TAG, "CN_DRV.product_id: %d"                       , temp_buf.product_id);
                    ESP_LOGI(TAG, "CN_DRV.last_frame_received_tick: %d"         , temp_buf.last_frame_received_tick);
                    ESP_LOGI(TAG, "CN_DRV.last_reset_reason: %d"                , temp_buf.last_reset_reason);
                    ESP_LOGI(TAG, "CN_DRV.logging_level: %d"                    , temp_buf.logging_level);
                }
            }
            break;
            case 1:
            {
                CN_DRV_Slot_partition_02_t temp_buf = {0};
                memcpy(&temp_buf, &frame.message.frame_bytes, sizeof(CN_DRV_Slot_partition_02_t));

                for (uint8_t i = 0; i < 3; i++)
                {
                    driver_slots[true_slot].channelConfig[i]           = temp_buf.channelConfig[i];

                    if (print_all_incoming_information)
                    {
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].channel: %d"        , i, temp_buf.channelConfig[i].channel);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].active: %d"         , i, temp_buf.channelConfig[i].active);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].slot: %d"           , i, temp_buf.channelConfig[i].slot);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].configured_type: %d", i, temp_buf.channelConfig[i].configured_type);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].current: %d"        , i, temp_buf.channelConfig[i].current);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].reset_current: %hn" , i, temp_buf.channelConfig[i].reset_current);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].nextBondedChannel: %p", i, temp_buf.channelConfig[i].nextBondedChannel);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].respondsToDali: %d" , i, temp_buf.channelConfig[i].respondsToDali);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].respondsToDMX: %d"  , i, temp_buf.channelConfig[i].respondsToDMX);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].bonded: %d"         , i, temp_buf.channelConfig[i].bonded);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].dmx_address: %d"    , i, temp_buf.channelConfig[i].dmx_address);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].dali_address: %d"   , i, temp_buf.channelConfig[i].dali_address);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].GW_Data.expose_channel_to_gateway: %d", i, temp_buf.channelConfig[i].GW_Data.expose_channel_to_gateway);
                        ESP_LOGI(TAG, "CN_DRV.channelConfig[%d].GW_Data.expose_channel_as_group: %d", i, temp_buf.channelConfig[i].GW_Data.expose_channel_as_group);
                    }
                }
            }
            break;
            case 2:
            {
                CN_DRV_Slot_partition_03_t temp_buf = {0};
                memcpy(&temp_buf, &frame.message.frame_bytes, sizeof(CN_DRV_Slot_partition_03_t));

                driver_slots[true_slot].channelConfig[3]          = temp_buf.channelConfig;

                if (print_all_incoming_information)
                {
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].channel: %d"             , temp_buf.channelConfig.channel);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].active: %d"              , temp_buf.channelConfig.active);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].slot: %d"                , temp_buf.channelConfig.slot);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].configured_type: %d"     , temp_buf.channelConfig.configured_type);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].current: %d"             , temp_buf.channelConfig.current);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].reset_current: %hn"      , temp_buf.channelConfig.reset_current);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].nextBondedChannel: %p"   , temp_buf.channelConfig.nextBondedChannel);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].respondsToDali: %d"      , temp_buf.channelConfig.respondsToDali);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].respondsToDMX: %d"       , temp_buf.channelConfig.respondsToDMX);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].bonded: %d"              , temp_buf.channelConfig.bonded);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].dmx_address: %d"         , temp_buf.channelConfig.dmx_address);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].dali_address: %d"        , temp_buf.channelConfig.dali_address);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].GW_Data.expose_channel_to_gateway: %d", temp_buf.channelConfig.GW_Data.expose_channel_to_gateway);
                    ESP_LOGI(TAG, "CN_DRV.channelConfig[3].GW_Data.expose_channel_as_group: %d", temp_buf.channelConfig.GW_Data.expose_channel_as_group);
                }

                for (uint8_t i = 0; i < DRIVER__MAX_CHANNELS_PER_DRIVER; i++)
                {
                    driver_slots[true_slot].channel_state[i]       = temp_buf.channel_state[i];

                    if (print_all_incoming_information)
                    {
                        ESP_LOGI(TAG, "CN_DRV.channel_state[%d].level: %d"          , i, temp_buf.channel_state[i].level);
                        ESP_LOGI(TAG, "CN_DRV.channel_state[%d].fault_mask: %d"     , i, temp_buf.channel_state[i].fault_mask);
                    }
                }
            }
            break;
            case 3:
            {
                CN_DRV_Slot_partition_04_t temp_buf = {0};
                memcpy(&temp_buf, &frame.message.frame_bytes, sizeof(CN_DRV_Slot_partition_04_t));

                driver_slots[true_slot].last_driver_metrics        = temp_buf.last_driver_metrics;

                if (print_all_incoming_information)
                {
                    ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.operational_minutes: %d"              , temp_buf.last_driver_metrics.operational_minutes);
                    ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.max_temperature: %d"                  , temp_buf.last_driver_metrics.max_temperature);
                    ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.temperature_zone_1_seconds: %d"       , temp_buf.last_driver_metrics.temperature_zone_1_seconds);
                    ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.temperature_zone_2_seconds: %d"       , temp_buf.last_driver_metrics.temperature_zone_2_seconds);
                    ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.temperature_zone_3_seconds: %d"       , temp_buf.last_driver_metrics.temperature_zone_3_seconds);
                    ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.temperature_zone_4_seconds: %d"       , temp_buf.last_driver_metrics.temperature_zone_4_seconds);
                    for (uint8_t i = 0; i < LM1_CHANNEL_COUNT; i++)
                    {
                        ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.channels[%d].output_seconds: %d"  , i, temp_buf.last_driver_metrics.channels[0].output_seconds);
                        ESP_LOGI(TAG, "CN_DRV.last_driver_metrics.channels[%d].watts_hours: %d"     , i, temp_buf.last_driver_metrics.channels[0].watts_hours);
                    }
                }
            }
            break;
            case 4:
            {
                CN_DRV_Slot_partition_05_t temp_buf = {0};
                memcpy(&temp_buf, &frame.message.frame_bytes, sizeof(CN_DRV_Slot_partition_05_t));

                for (uint8_t i = 0; i < 41; i++)
                {
                    driver_slots[true_slot].git_commmit_hash[i]        = temp_buf.git_commmit_hash[i];
                }

                if (print_all_incoming_information)
                {
                    ESP_LOGI(TAG, "CN_DRV.git_commmit_hash: %s"         , temp_buf.git_commmit_hash);
                }
            }
            break;
            default:
                break;
            }

            if (frame.message.slot < 39)
            {
                next_expected_slot = frame.message.slot + 1;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Driver;
            }
            else
            {
                next_expected_slot = 0;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Wired_Switch;
            }
        }
        break;

        case CNGW_CONFIG_CMD_Config_Info_Wired_Switch:
        {
            memcpy(&wired_sw_configuration[frame.message.slot], &frame.message.frame_bytes, sizeof(struct CN_Switch_Configuration_t));
            if (print_all_incoming_information)
            {
                ESP_LOGI(TAG, "wired_sw_configuration.parameters: %d"               , wired_sw_configuration[frame.message.slot].parameters);
                ESP_LOGI(TAG, "wired_sw_configuration.type: %d"                     , wired_sw_configuration[frame.message.slot].type);
                ESP_LOGI(TAG, "wired_sw_configuration.lastReceivedTrigger: %d"      , wired_sw_configuration[frame.message.slot].lastReceivedTrigger);
                ESP_LOGI(TAG, "wired_sw_configuration.id: %d"                       , wired_sw_configuration[frame.message.slot].id);
                ESP_LOGI(TAG, "wired_sw_configuration.flags: %d"                    , wired_sw_configuration[frame.message.slot].flags);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[0].eventTrigger: %d" , wired_sw_configuration[frame.message.slot].commands[0].eventTrigger);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[0].reserved: %d"     , wired_sw_configuration[frame.message.slot].commands[0].reserved);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[0].action: %d"       , wired_sw_configuration[frame.message.slot].commands[0].action);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[0].parameters: %d"   , wired_sw_configuration[frame.message.slot].commands[0].parameters);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[1].eventTrigger: %d" , wired_sw_configuration[frame.message.slot].commands[1].eventTrigger);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[1].reserved: %d"     , wired_sw_configuration[frame.message.slot].commands[1].reserved);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[1].action: %d"       , wired_sw_configuration[frame.message.slot].commands[1].action);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[1].parameters: %d"   , wired_sw_configuration[frame.message.slot].commands[1].parameters);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[2].eventTrigger: %d" , wired_sw_configuration[frame.message.slot].commands[2].eventTrigger);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[2].reserved: %d"     , wired_sw_configuration[frame.message.slot].commands[2].reserved);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[2].action: %d"       , wired_sw_configuration[frame.message.slot].commands[2].action);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[2].parameters: %d"   , wired_sw_configuration[frame.message.slot].commands[2].parameters);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[3].eventTrigger: %d" , wired_sw_configuration[frame.message.slot].commands[3].eventTrigger);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[3].reserved: %d"     , wired_sw_configuration[frame.message.slot].commands[3].reserved);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[3].action: %d"       , wired_sw_configuration[frame.message.slot].commands[3].action);
                ESP_LOGI(TAG, "wired_sw_configuration.commands[3].parameters: %d"   , wired_sw_configuration[frame.message.slot].commands[3].parameters);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[0].targetType: %d"    , wired_sw_configuration[frame.message.slot].targets[0].targetType);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[0].cabinetNumber: %d" , wired_sw_configuration[frame.message.slot].targets[0].cabinetNumber);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[0].slot: %d"          , wired_sw_configuration[frame.message.slot].targets[0].slot);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[0].channel: %d"       , wired_sw_configuration[frame.message.slot].targets[0].channel);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[0].group: %d"         , wired_sw_configuration[frame.message.slot].targets[0].group);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[0].scene: %d"         , wired_sw_configuration[frame.message.slot].targets[0].scene);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[1].targetType: %d"    , wired_sw_configuration[frame.message.slot].targets[1].targetType);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[1].cabinetNumber: %d" , wired_sw_configuration[frame.message.slot].targets[1].cabinetNumber);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[1].slot: %d"          , wired_sw_configuration[frame.message.slot].targets[1].slot);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[1].channel: %d"       , wired_sw_configuration[frame.message.slot].targets[1].channel);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[1].group: %d"         , wired_sw_configuration[frame.message.slot].targets[1].group);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[1].scene: %d"         , wired_sw_configuration[frame.message.slot].targets[1].scene);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[2].targetType: %d"    , wired_sw_configuration[frame.message.slot].targets[2].targetType);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[2].cabinetNumber: %d" , wired_sw_configuration[frame.message.slot].targets[2].cabinetNumber);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[2].slot: %d"          , wired_sw_configuration[frame.message.slot].targets[2].slot);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[2].channel: %d"       , wired_sw_configuration[frame.message.slot].targets[2].channel);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[2].group: %d"         , wired_sw_configuration[frame.message.slot].targets[2].group);
                ESP_LOGI(TAG, "wired_sw_configuration.targets[2].scene: %d"         , wired_sw_configuration[frame.message.slot].targets[2].scene);
            }

            if (frame.message.slot < WIRED_SWITCH_COUNT - 1)
            {
                next_expected_slot = frame.message.slot + 1;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Wired_Switch;
            }
            else
            {
                next_expected_slot = 0;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Wireless_Switch;
            }
        }
        break;

        case CNGW_CONFIG_CMD_Config_Info_Wireless_Switch:
        {
            memcpy(&wireless_sw_configuration[frame.message.slot], &frame.message.frame_bytes, sizeof(struct CN_Switch_Configuration_t));

            if (print_all_incoming_information)
            {
                ESP_LOGI(TAG, "wireless_sw_configuration.parameters: %d"                , wireless_sw_configuration[frame.message.slot].parameters);
                ESP_LOGI(TAG, "wireless_sw_configuration.type: %d"                      , wireless_sw_configuration[frame.message.slot].type);
                ESP_LOGI(TAG, "wireless_sw_configuration.lastReceivedTrigger: %d"       , wireless_sw_configuration[frame.message.slot].lastReceivedTrigger);
                ESP_LOGI(TAG, "wireless_sw_configuration.id: %d"                        , wireless_sw_configuration[frame.message.slot].id);
                ESP_LOGI(TAG, "wireless_sw_configuration.flags: %d"                     , wireless_sw_configuration[frame.message.slot].flags);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[0].eventTrigger: %d"  , wireless_sw_configuration[frame.message.slot].commands[0].eventTrigger);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[0].reserved: %d"      , wireless_sw_configuration[frame.message.slot].commands[0].reserved);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[0].action: %d"        , wireless_sw_configuration[frame.message.slot].commands[0].action);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[0].parameters: %d"    , wireless_sw_configuration[frame.message.slot].commands[0].parameters);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[1].eventTrigger: %d"  , wireless_sw_configuration[frame.message.slot].commands[1].eventTrigger);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[1].reserved: %d"      , wireless_sw_configuration[frame.message.slot].commands[1].reserved);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[1].action: %d"        , wireless_sw_configuration[frame.message.slot].commands[1].action);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[1].parameters: %d"    , wireless_sw_configuration[frame.message.slot].commands[1].parameters);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[2].eventTrigger: %d"  , wireless_sw_configuration[frame.message.slot].commands[2].eventTrigger);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[2].reserved: %d"      , wireless_sw_configuration[frame.message.slot].commands[2].reserved);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[2].action: %d"        , wireless_sw_configuration[frame.message.slot].commands[2].action);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[2].parameters: %d"    , wireless_sw_configuration[frame.message.slot].commands[2].parameters);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[3].eventTrigger: %d"  , wireless_sw_configuration[frame.message.slot].commands[3].eventTrigger);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[3].reserved: %d"      , wireless_sw_configuration[frame.message.slot].commands[3].reserved);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[3].action: %d"        , wireless_sw_configuration[frame.message.slot].commands[3].action);
                ESP_LOGI(TAG, "wireless_sw_configuration.commands[3].parameters: %d"    , wireless_sw_configuration[frame.message.slot].commands[3].parameters);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[0].targetType: %d"     , wireless_sw_configuration[frame.message.slot].targets[0].targetType);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[0].cabinetNumber: %d"  , wireless_sw_configuration[frame.message.slot].targets[0].cabinetNumber);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[0].slot: %d"           , wireless_sw_configuration[frame.message.slot].targets[0].slot);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[0].channel: %d"        , wireless_sw_configuration[frame.message.slot].targets[0].channel);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[0].group: %d"          , wireless_sw_configuration[frame.message.slot].targets[0].group);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[0].scene: %d"          , wireless_sw_configuration[frame.message.slot].targets[0].scene);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[1].targetType: %d"     , wireless_sw_configuration[frame.message.slot].targets[1].targetType);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[1].cabinetNumber: %d"  , wireless_sw_configuration[frame.message.slot].targets[1].cabinetNumber);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[1].slot: %d"           , wireless_sw_configuration[frame.message.slot].targets[1].slot);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[1].channel: %d"        , wireless_sw_configuration[frame.message.slot].targets[1].channel);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[1].group: %d"          , wireless_sw_configuration[frame.message.slot].targets[1].group);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[1].scene: %d"          , wireless_sw_configuration[frame.message.slot].targets[1].scene);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[2].targetType: %d"     , wireless_sw_configuration[frame.message.slot].targets[2].targetType);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[2].cabinetNumber: %d"  , wireless_sw_configuration[frame.message.slot].targets[2].cabinetNumber);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[2].slot: %d"           , wireless_sw_configuration[frame.message.slot].targets[2].slot);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[2].channel: %d"        , wireless_sw_configuration[frame.message.slot].targets[2].channel);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[2].group: %d"          , wireless_sw_configuration[frame.message.slot].targets[2].group);
                ESP_LOGI(TAG, "wireless_sw_configuration.targets[2].scene: %d"          , wireless_sw_configuration[frame.message.slot].targets[2].scene);
            }

            if (frame.message.slot < WIRELESS_SWITCH_COUNT - 1)
            {
                next_expected_slot = frame.message.slot + 1;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Wireless_Switch;
            }
            else
            {
                next_expected_slot = 0;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Sensor;
            }
        }
        break;

        case CNGW_CONFIG_CMD_Config_Info_Sensor:
        {
            memcpy(&sensor_configuration[frame.message.slot], &frame.message.frame_bytes, sizeof(CN_Sensor_Configuration_t));
            if (print_all_incoming_information)
            {
                ESP_LOGI(TAG, "sensor_configuration.type: %d"                           , sensor_configuration[frame.message.slot].type);
                ESP_LOGI(TAG, "sensor_configuration.last_trigger_tick: %d"              , sensor_configuration[frame.message.slot].last_trigger_tick);
                ESP_LOGI(TAG, "sensor_configuration.id: %d"                             , sensor_configuration[frame.message.slot].id);
                ESP_LOGI(TAG, "sensor_configuration.commands[0].eventTrigger: %d"       , sensor_configuration[frame.message.slot].commands[0].eventTrigger);
                ESP_LOGI(TAG, "sensor_configuration.commands[0].reserved: %d"           , sensor_configuration[frame.message.slot].commands[0].reserved);
                ESP_LOGI(TAG, "sensor_configuration.commands[0].action: %d"             , sensor_configuration[frame.message.slot].commands[0].action);
                ESP_LOGI(TAG, "sensor_configuration.commands[0].parameters: %d"         , sensor_configuration[frame.message.slot].commands[0].parameters);
                ESP_LOGI(TAG, "sensor_configuration.commands[1].eventTrigger: %d"       , sensor_configuration[frame.message.slot].commands[1].eventTrigger);
                ESP_LOGI(TAG, "sensor_configuration.commands[1].reserved: %d"           , sensor_configuration[frame.message.slot].commands[1].reserved);
                ESP_LOGI(TAG, "sensor_configuration.commands[1].action: %d"             , sensor_configuration[frame.message.slot].commands[1].action);
                ESP_LOGI(TAG, "sensor_configuration.commands[1].parameters: %d"         , sensor_configuration[frame.message.slot].commands[1].parameters);
                ESP_LOGI(TAG, "sensor_configuration.targets[0].targetType: %d"          , sensor_configuration[frame.message.slot].targets[0].targetType);
                ESP_LOGI(TAG, "sensor_configuration.targets[0].cabinetNumber: %d"       , sensor_configuration[frame.message.slot].targets[0].cabinetNumber);
                ESP_LOGI(TAG, "sensor_configuration.targets[0].slot: %d"                , sensor_configuration[frame.message.slot].targets[0].slot);
                ESP_LOGI(TAG, "sensor_configuration.targets[0].channel: %d"             , sensor_configuration[frame.message.slot].targets[0].channel);
                ESP_LOGI(TAG, "sensor_configuration.targets[0].group: %d"               , sensor_configuration[frame.message.slot].targets[0].group);
                ESP_LOGI(TAG, "sensor_configuration.targets[0].scene: %d"               , sensor_configuration[frame.message.slot].targets[0].scene);
                ESP_LOGI(TAG, "sensor_configuration.targets[1].targetType: %d"          , sensor_configuration[frame.message.slot].targets[1].targetType);
                ESP_LOGI(TAG, "sensor_configuration.targets[1].cabinetNumber: %d"       , sensor_configuration[frame.message.slot].targets[1].cabinetNumber);
                ESP_LOGI(TAG, "sensor_configuration.targets[1].slot: %d"                , sensor_configuration[frame.message.slot].targets[1].slot);
                ESP_LOGI(TAG, "sensor_configuration.targets[1].channel: %d"             , sensor_configuration[frame.message.slot].targets[1].channel);
                ESP_LOGI(TAG, "sensor_configuration.targets[1].group: %d"               , sensor_configuration[frame.message.slot].targets[1].group);
                ESP_LOGI(TAG, "sensor_configuration.targets[1].scene: %d"               , sensor_configuration[frame.message.slot].targets[1].scene);
                ESP_LOGI(TAG, "sensor_configuration.targets[2].targetType: %d"          , sensor_configuration[frame.message.slot].targets[2].targetType);
                ESP_LOGI(TAG, "sensor_configuration.targets[2].cabinetNumber: %d"       , sensor_configuration[frame.message.slot].targets[2].cabinetNumber);
                ESP_LOGI(TAG, "sensor_configuration.targets[2].slot: %d"                , sensor_configuration[frame.message.slot].targets[2].slot);
                ESP_LOGI(TAG, "sensor_configuration.targets[2].channel: %d"             , sensor_configuration[frame.message.slot].targets[2].channel);
                ESP_LOGI(TAG, "sensor_configuration.targets[2].group: %d"               , sensor_configuration[frame.message.slot].targets[2].group);
                ESP_LOGI(TAG, "sensor_configuration.targets[2].scene: %d"               , sensor_configuration[frame.message.slot].targets[2].scene);
                ESP_LOGI(TAG, "sensor_configuration.parameters.desired_lux: %d"         , sensor_configuration[frame.message.slot].parameters.desired_lux);
                ESP_LOGI(TAG, "sensor_configuration.parameters.occupancy_timeout_minutes: %d", sensor_configuration[frame.message.slot].parameters.occupancy_timeout_minutes);
            }

            if (frame.message.slot < MAX_SENSORS - 1)
            {
                next_expected_slot = frame.message.slot + 1;
                next_expected_command = CNGW_CONFIG_CMD_Config_Info_Sensor;
            }
            else
            {
                next_expected_slot = 0;
                ESP_LOGI(TAG, "All configurations successfully copied");
                for (int i = 0; i < CABINET__DRIVER_SLOT_COUNT; i++)
                {
                    for (int j = 0; j < DRIVER__MAX_CHANNELS_PER_DRIVER; j++)
                    {
                        ESP_LOGI(TAG, "driver_slots[%d]->channelConfig[%d].current: %d"     , i, j, driver_slots[i].channelConfig[j].current);
                    }
                }
                Send_GW_message_to_AWS(64, 0, "All configurations successfully copied");
                exit_task = true;
            }
        }
        break;
        default:
            exit_task = true;
            break;
        }
        
        resume_task = true;
    }
}
#endif