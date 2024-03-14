/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: SpacrGateway_commands.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: main C file for the GW. Handles the GW initialization. 
 * This file includes functions to handle commands and messages coming from ROOT,
 * prepare the response message to AWS, and GW initialization.
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#include "mbedtls/sha256.h"
#ifdef GATEWAY_ETHERNET
#include "includes/root_utilities.h"
#endif
//#include "esp_heap_caps.h"

static const char *TAG = "GW_command";
#ifdef GATEWAY_DEBUGGING
static bool allow_same_FW_to_be_flashed = true;
static bool print_all_frame_info        = true;
#else
static bool allow_same_FW_to_be_flashed = false;
static bool print_all_frame_info        = false;
#endif

// define and initialize all the main structs used in the GW
struct CN_Switch_Configuration_t wired_sw_configuration[WIRED_SWITCH_COUNT]         = { 0 };
struct CN_Switch_Configuration_t wireless_sw_configuration[WIRELESS_SWITCH_COUNT]   = { 0 };
CNGW_CN_Board_Info_t cn_board_info                                                  = { 0 };  
CN_Sensor_Configuration_t sensor_configuration[MAX_SENSORS]                         = { 0 };
CN_DRV_Slot_t driver_slots[CABINET__DRIVER_SLOT_COUNT]                              = { 0 };


//OTA related variables
size_t total_received_data                  = 0;
size_t total_expected_data                  = 0;
uint8_t next_expected_sequence_number       = 0;
bool OTA_data_expected                      = false;
bool OTA_overall_status                     = false;
CNGW_Firmware_Binary_Type target_MCU_int    = CNGW_FIRMWARE_BINARY_TYPE_invalid;


/**
 * @brief creates a JSON struct from the arguements and send it to the ROOT/ SIM7080 to be sent to AWS
 * @param ubyCommand[in] the command number
 * @param uwValue[in] the integer value
 * @param cptrString[in] the string
 */
void Send_GW_message_to_AWS(uint16_t ubyCommand, uint32_t uwValue, char *cptrString)
{
    LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_COMM, LED_CHANGE_RAPID_DURATION);

#if defined(GATEWAY_SIM7080) || defined(GATEWAY_ETHERNET)
    SecondaryUtilities_PrepareJSONAndSendToAWS(ubyCommand, uwValue , cptrString);
#endif
}
/**
 * @brief Initialize all necessary tasks for GW
 */
void Initialize_Gateway()
{
    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "Gateway initializing ...");
    }

    // 0. get the HW and FW version
    update_GW_version();
    ESP_LOGW(TAG, "GW HW v%d.%d.%d",    cn_board_info.gw_mcu.hardware_version.major,    cn_board_info.gw_mcu.hardware_version.minor,    cn_board_info.gw_mcu.hardware_version.ci);
    ESP_LOGW(TAG, "GW FW v%d.%d.%d-%d", cn_board_info.gw_mcu.application_version.major, cn_board_info.gw_mcu.application_version.minor, cn_board_info.gw_mcu.application_version.ci, cn_board_info.gw_mcu.application_version.branch_id);


    esp_err_t result = ESP_FAIL;
    // 1. initialize GPIO
    result = init_output_GPIO();
    if(result != ESP_OK)
    {
        error_handler("init_output_GPIO", result);
    }
    result = init_input_power_GPIO();
    if(result != ESP_OK)
    {
        error_handler("init_input_power_GPIO", result);
    }

    // 2. initialize i2c resources
    result = init_I2C();
    if(result != ESP_OK)
    {
        error_handler("init_I2C", result);
    }

    // 3. initialize status LEDs
    result = init_status_LEDs();
    if(result != ESP_OK)
    {
        error_handler("init_status_LEDs", result);
    }

    // 4. provision the security chip if needed
#ifdef GATEWAY_PROVISION_ATMEL508
    result = ATMEL508_provision_chip();
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure security chip.  Halting...");
        error_handler("ATMEL508_provision_chip", result);
    }
#endif

    // 5. initialize OTA variables
    OTA_Init(OTA_DEFAULT_SESSION_TIMEOUT_TICKS);

    // 6. initialize SPI communication
    result = init_GW_SPI_communication();
    if(result != ESP_OK)
    {
        error_handler("init_GW_SPI_communication", result);
    }

    // 7. keep on checking for GW availability
    check_for_GW_availability();

    // 8. Init SIM module
#ifdef GATEWAY_SIM7080
    // initialize the SIM7080 module
    result = init_SIM7080();
    if(result != ESP_OK)
    {
        error_handler("init_SIM7080", result);
    }
#ifdef GATEWAY_SIM7080_ADD_CERTIFICATES
    // if needed. add the certificates to the SIM memory
    result = Send_Certs_To_SIM();
    if(result != ESP_OK)
    {
        error_handler("Send_Certs_To_SIM", result);
    }
#endif
    // connect to the internet
    SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(500);
#endif

}

/**
 * @brief Parse the frame to perform Action commands. All the types of actions are handled here
 * @param structNodeReceived[in] JSON data from the ROOT
 * @return true if the message has a valid byte size
 */
bool GW_Process_Action_Command_As_ByteArray(NodeStruct_t *structNodeReceived)
{
    if(print_all_frame_info)
    {
        ESP_LOGW(TAG, "GW_Process_Action_Command_As_ByteArray Function Called");
        ESP_LOGI(TAG, "structNodeReceived->cptrString: %s", structNodeReceived->cptrString);
    }

    size_t strLength = strlen(structNodeReceived->cptrString);
    if (strLength % 2 != 0)
    {
        ESP_LOGE(TAG, "Invalid byte size");
        return false;
    }
    else
    {
        // Calculate the number of bytes in the original byte array
        size_t byteArrayLength = strLength / 2;
        // Create a byte array to hold the decoded values
        unsigned char byteArray[byteArrayLength];
        // Convert the string to a byte array
        for (size_t i = 0; i < byteArrayLength; i++)
        {
            sscanf(structNodeReceived->cptrString + 2 * i, "%2hhx", &byteArray[i]);
        }
        consume_GW_message((uint8_t *)&byteArray);
    }
    return true;
}

/**
 * @brief Parse the frame to perform Action commands. All the types of actions are handled here
 * @param structNodeReceived[in] JSON data from the ROOT
 * @return true if the message has a valid byte size
 */
bool GW_Process_Action_Command(NodeStruct_t *structNodeReceived)
{
    LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_COMM, LED_CHANGE_MOMENTARY_DURATION);
    bool result = true;

    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "GW_Process_Action_Command Function Called");
    }

    cJSON *json = cJSON_Parse(structNodeReceived->cptrString);
    // Action validity checks
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Action command JSON not found");
        result = false;
    }
    else
    {
        cJSON *cjAddress = cJSON_GetObjectItemCaseSensitive(json, "address");
        if (cJSON_IsNumber(cjAddress))
        {
            if (cjAddress->valueint >= 0 && cjAddress->valueint < NUM_DR_CHANNELS)
            {
                if (structNodeReceived->dValue >= 0.00 && structNodeReceived->dValue <= 100.00)
                {

                    cJSON *cjCommand = cJSON_GetObjectItemCaseSensitive(json, "command");
                    if (cJSON_IsNumber(cjCommand))
                    {
                        cJSON *cjAddress_type = cJSON_GetObjectItemCaseSensitive(json, "address_type");
                        if (cJSON_IsNumber(cjAddress_type))
                        {
                            cJSON *cjFade_unit = cJSON_GetObjectItemCaseSensitive(json, "fade_unit");
                            if (cJSON_IsNumber(cjFade_unit))
                            {

                                cJSON *cjFade_time = cJSON_GetObjectItemCaseSensitive(json, "fade_time");
                                if (cJSON_IsNumber(cjFade_time))
                                {
                                    LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_CN, LED_CHANGE_MOMENTARY_DURATION);

                                    CNGW_Action_Frame_t actionFrame = {0};
                                    CCP_UTIL_Get_Msg_Header(&actionFrame.header, CNGW_HEADER_TYPE_Action_Commmand, sizeof(actionFrame.message));

                                    actionFrame.message.command = cjCommand->valueint;
                                    actionFrame.message.address.target_cabinet = cn_board_info.cn_config.cabinet_number;
                                    actionFrame.message.address.address_type = cjAddress_type->valueint;
                                    actionFrame.message.address.target_address = cjAddress->valueint;
                                    actionFrame.message.action_parameters.fade_unit = cjFade_unit->valueint;
                                    actionFrame.message.action_parameters.fade_time = cjFade_time->valueint;
                                    actionFrame.message.action_parameters.light_level = (int)((structNodeReceived->dValue) * (2047.00) / (100.00));
                                    actionFrame.message.action_parameters.reserved = 0;

                                    actionFrame.message.crc = CCP_UTIL_Get_Crc8(0, (uint8_t *)&(actionFrame.message), sizeof(actionFrame.message) - sizeof(actionFrame.message.crc));
                                    consume_GW_message((uint8_t *)&actionFrame);
                                }
                                else
                                {
                                    ESP_LOGE(TAG, "Action command invalid fade time");
                                    result = false;
                                }
                            }
                            else
                            {
                                ESP_LOGE(TAG, "Action command invalid fade unit");
                                result = false;
                            }
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Action command invalid address type");
                            result = false;
                        }
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Action command invalid command");
                        result = false;
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "Action command invalid light level");
                    result = false;
                }
            }
            else
            {
                ESP_LOGE(TAG, "Action command invalid address");
                result = false;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Action command address not found");
            result = false;
        }

        cJSON_Delete(json);
    }

    return result;
}

/**
 * @brief Parse the frame to execute AT commands.
 * @param structNodeReceived[in] JSON data from the ROOT
 * @return true if the message has a valid byte size
 */
bool GW_Process_AT_Command(NodeStruct_t *structNodeReceived)
{
    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "GW_Process_AT_Command Function Called");
    }

    cJSON *json = cJSON_Parse(structNodeReceived->cptrString);
    // Command validity checks
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Action command JSON not found");
        return false;
    }
    else
    {
        cJSON *cjString = cJSON_GetObjectItemCaseSensitive(json, "AT_cmd");
        if (cJSON_IsString(cjString))
        {
#ifdef GATEWAY_SIM7080
            Execute_AT_CMD(cjString->valuestring, structNodeReceived->dValue);
#endif
        }

        cJSON_Delete(json);
    }
    return true;
}

bool Send_Response_To_AWS(CNGW_AWS_Response_t *data, uint8_t size)
{
    uint8_t required_size = size + sizeof(cn_board_info.cn_mcu.serial) + sizeof(CNGW_AWS_Command);
    unsigned char byteArray[sizeof(CNGW_AWS_Response_t)] = {0};
    memcpy(byteArray, data, required_size);
    char hexString[required_size * 2 + 1];
    char *hexPtr = hexString; // Initialize a pointer to the start of hexString
    for (size_t i = 0; i < required_size; i++)
    {
        sprintf(hexPtr, "%02X", byteArray[i]);
        hexPtr += 2; // Move the pointer to the next position
    }
    // Null-terminate the string
    hexString[required_size * 2] = '\0';

    Send_GW_message_to_AWS(64, 0, hexString);

    if(print_all_frame_info)
    {
        ESP_LOGI(TAG, "Send_Response_To_AWS: %s", hexString);
    }

    return true;
}


bool GW_Process_Query_Command(NodeStruct_t *structNodeReceived)
{
    LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_COMM, LED_CHANGE_MOMENTARY_DURATION);

    if(print_all_frame_info)
    {
        ESP_LOGI(TAG, "GW_Process_Query_Command Function Called");
    }

    CNGW_AWS_Response_t AWS_Response = {0};
    memcpy(AWS_Response.CN_Serial, &cn_board_info.cn_mcu.serial, sizeof(cn_board_info.cn_mcu.serial));

    if (strcmp(structNodeReceived->cptrString, "Update_Message") == 0)
    {
        switch ((int)structNodeReceived->dValue)
        {
        case 0:
            // cn mcu query
            AWS_Response.message_type = CNGW_AWS_CMD_UPDATE_CN;
            memcpy(&AWS_Response.info_update, &cn_board_info.cn_mcu, sizeof(CNGW_Device_Info_Update_Message_t));
            break;
        case 1:
            // sw mcu query
            AWS_Response.message_type = CNGW_AWS_CMD_UPDATE_SW;
            memcpy(&AWS_Response.info_update, &cn_board_info.sw_mcu, sizeof(CNGW_Device_Info_Update_Message_t));
            break;
        case 2:
            // gw mcu query
            AWS_Response.message_type = CNGW_AWS_CMD_UPDATE_GW;
            memcpy(&AWS_Response.info_update, &cn_board_info.gw_mcu, sizeof(CNGW_Device_Info_Update_Message_t));
            break;
        default:
            // dr mcu query
            if ((int)structNodeReceived->dValue >= 3 && (int)structNodeReceived->dValue < 11)
            {
                AWS_Response.message_type = CNGW_AWS_CMD_UPDATE_DR;
                memcpy(&AWS_Response.info_update, &cn_board_info.dr_mcu[(int)structNodeReceived->dValue - 3], sizeof(CNGW_Device_Info_Update_Message_t));
            }
            else
            {
                return false;
            }
        }
        return Send_Response_To_AWS(&AWS_Response, sizeof(CNGW_Device_Info_Update_Message_t));
    }
    
    else if (strcmp(structNodeReceived->cptrString, "Attribute_Message") == 0)
    {
        if ((int)structNodeReceived->dValue >= 0 && (int)structNodeReceived->dValue < NUM_DR_CHANNELS)
        {
            AWS_Response.message_type = CNGW_AWS_CMD_Attribute;
            memcpy(&AWS_Response.attribute, &cn_board_info.channel_attribute[(int)structNodeReceived->dValue], sizeof(CNGW_Update_Attribute_Message_t));
            return Send_Response_To_AWS(&AWS_Response, sizeof(CNGW_Update_Attribute_Message_t));
        }
        else
        {
            return false;
        }
    }
    
    else if (strcmp(structNodeReceived->cptrString, "Channel_Status_Message") == 0)
    {
        if ((int)structNodeReceived->dValue >= 0 && (int)structNodeReceived->dValue < NUM_DR_CHANNELS)
        {
            AWS_Response.message_type = CNGW_AWS_CMD_Status;
            memcpy(&AWS_Response.channel_status, &cn_board_info.channel_status[(int)structNodeReceived->dValue], sizeof(CNGW_Update_Channel_Status_Message_t));
            return Send_Response_To_AWS(&AWS_Response, sizeof(CNGW_Update_Channel_Status_Message_t));
        }
        else
        {
            return false;
        }
    }
    
    else if (strcmp(structNodeReceived->cptrString, "General_Config") == 0)
    {
        AWS_Response.message_type = CNGW_AWS_CMD_Gen_Config;
        memcpy(&AWS_Response.gen_config, &cn_board_info.cn_config, sizeof(CN_General_Config_t));
        return Send_Response_To_AWS(&AWS_Response, sizeof(CN_General_Config_t));
    }
    
    else if (strcmp(structNodeReceived->cptrString, "Query_All_Channel") == 0)
    {
        query_all_channel_info();
    }
    
    else if (strcmp(structNodeReceived->cptrString, "Get_Config_Info") == 0)
    {
        Request_Configuration_information();
    }

    else if (strcmp(structNodeReceived->cptrString, "Restart_Gateway") == 0)
    {
        delayed_ESP_Restart((int)structNodeReceived->dValue);
    }

    else if (strcmp(structNodeReceived->cptrString, "Restart_Mainboard") == 0)
    {
        ESP_LOGW(TAG, "Restarting the mainboard now...");
        LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_CN, LED_CHANGE_MOMENTARY_DURATION);
        send_direct_control_message(CNGW_DIRECT_CONTROL_CMD_Mainboard_Restart, CNGW_SOURCE_MCU_CN, 0, 0, 0);
    }

    else if (strstr(structNodeReceived->cptrString, "Change_Driver_Current_DR") != NULL)
    {
        CNGW_Source_MCU target_DR;
        uint8_t target_CH;

        if (sscanf(structNodeReceived->cptrString, "Change_Driver_Current_DR%dCH%hhu", (int *)&target_DR, &target_CH) == 2)
        {
            if (target_DR > 8)
            {
                return false;
            }
            if (target_CH > 3)
            {
                return false;
            }
            ESP_LOGW(TAG, "Sending CNGW_DIRECT_CONTROL_CMD message to driver %u, channel %hhu to panel", target_DR, target_CH);
            LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_CN, LED_CHANGE_MOMENTARY_DURATION);
            send_direct_control_message(CNGW_DIRECT_CONTROL_CMD_Driver_Change_Current, target_DR + 3, target_CH, (int)structNodeReceived->dValue, 0);
        }
        else
        {
            return false;
        }
    }


    else
    {
        return false;
    }

    return true;
}



/**
 * @brief OTA begin command from the ROOT. Get the total data length to be expected, set OTA_data_expected = true, identify the target MCU and the version number.
 * @param structNodeReceived[in] JSON data from the ROOT
 * @return true (for now)
 */
bool GW_Process_OTA_Command_Begin(NodeStruct_t *structNodeReceived)
{
    // first check if the Gateway is successfully connected to a mainboard
    if(!cn_board_info.cn_config.cn_is_handshake)
    {
        ESP_LOGE(TAG, "The Gateway is not connected to a CENCE mainboard. Aborting FW update");
        return false;
    }
    Send_GW_message_to_AWS(64, 0, "Begin OTA");


    total_received_data             = 0;
    total_expected_data             = (int)structNodeReceived->dValue;
    next_expected_sequence_number   = 0;
    OTA_data_expected               = true;
    OTA_overall_status              = true;
    // find out the type of the target MCU and it's version
    char target_MCU[50];
    target_MCU_int      = CNGW_FIRMWARE_BINARY_TYPE_invalid;
    int major_version   = 0;
    int minor_version   = 0;
    int ci_version      = 0;
    int branch_id       = 0;
    ESP_LOGW(TAG, "structNodeReceived->cptrString: %s", structNodeReceived->cptrString);
    if (sscanf(structNodeReceived->cptrString, "%[^-]-v%d.%d.%d.%d.", target_MCU, &major_version, &minor_version, &ci_version, &branch_id) == 5)
    {
        if (strcmp(target_MCU, "cense_cn_mcu") == 0)
        {
            target_MCU_int = CNGW_FIRMWARE_BINARY_TYPE_cn_mcu;
            ESP_LOGW(TAG, "Current FW version: %d.%d.%d-%d", cn_board_info.cn_mcu.application_version.major, cn_board_info.cn_mcu.application_version.minor, cn_board_info.cn_mcu.application_version.ci, cn_board_info.cn_mcu.application_version.branch_id);
            ESP_LOGW(TAG, "Incoming FW version: %d.%d.%d-%d", major_version, minor_version, ci_version, branch_id);
            if (major_version > cn_board_info.cn_mcu.application_version.major)
            {
                //incoming major version is greater than the current one. Do FW update
                if(print_all_frame_info)
                {
                    ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                }
            }
            else
            {
                if (major_version == cn_board_info.cn_mcu.application_version.major)
                {
                    // incoming major version is equal to the current one. check furthur
                    if (minor_version > cn_board_info.cn_mcu.application_version.minor)
                    {
                        // incoming minor version is greater than the current one. Do FW update
                        if (print_all_frame_info)
                        {
                            ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                        }
                    }
                    else
                    {
                        if (minor_version == cn_board_info.cn_mcu.application_version.minor)
                        {
                            // incoming minor version is equal to the current one. check furthur
                            if (allow_same_FW_to_be_flashed)
                            {
                                if (ci_version >= cn_board_info.cn_mcu.application_version.ci)
                                {
                                    // incoming ci version is greater or equal to the current one. Do FW update
                                    if (print_all_frame_info)
                                    {
                                        ESP_LOGI(TAG, "Incoming FW version is same/ newer than the Current FW version");
                                    }
                                }
                                else
                                {
                                    // incoming ci version is less than the current one. Don't FW update
                                    ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                    OTA_overall_status = false;
                                }
                            }
                            else
                            { // incoming ci version is greater than the current one. Do FW update
                                if (ci_version > cn_board_info.cn_mcu.application_version.ci)
                                {
                                    // incoming ci version is greater or equal to the current one. Do FW update
                                    if (print_all_frame_info)
                                    {
                                        ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                                    }
                                }
                                else
                                { // incoming ci version is equal or less than the current one. Don't FW update
                                    if (ci_version == cn_board_info.cn_mcu.application_version.ci)
                                    {
                                        ESP_LOGE(TAG, "Incoming FW version is same as the Current FW version");
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                    }
                                    OTA_overall_status = false;
                                }
                            }
                        }
                        else
                        {
                            // incoming minor version is less than the current one. Don't FW update
                            ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                            OTA_overall_status = false;
                        }
                    }
                }
                else
                {
                    //incoming major version is less than the current one. Don't FW update
                    ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                    OTA_overall_status = false;
                }
            }
        }
        else if (strcmp(target_MCU, "cense_dr_mcu") == 0)
        {
            target_MCU_int = CNGW_FIRMWARE_BINARY_TYPE_dr_mcu;
        }
        else if (strcmp(target_MCU, "cense_sw_mcu") == 0)
        {
            target_MCU_int = CNGW_FIRMWARE_BINARY_TYPE_sw_mcu;
            ESP_LOGW(TAG, "Current FW version: %d.%d.%d-%d", cn_board_info.sw_mcu.application_version.major, cn_board_info.sw_mcu.application_version.minor, cn_board_info.sw_mcu.application_version.ci, cn_board_info.sw_mcu.application_version.branch_id);
            ESP_LOGW(TAG, "Incoming FW version: %d.%d.%d-%d", major_version, minor_version, ci_version, branch_id);
            if (major_version > cn_board_info.sw_mcu.application_version.major)
            {
                //incoming major version is greater than the current one. Do FW update
                ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
            }
            else
            {
                if (major_version == cn_board_info.sw_mcu.application_version.major)
                {
                    // incoming major version is equal to the current one. check furthur
                    if (minor_version > cn_board_info.sw_mcu.application_version.minor)
                    {
                        // incoming minor version is greater than the current one. Do FW update
                        ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                    }
                    else
                    {
                        if (minor_version == cn_board_info.sw_mcu.application_version.minor)
                        {
                            // incoming minor version is equal to the current one. check furthur
                            if (allow_same_FW_to_be_flashed)
                            {
                                if (ci_version >= cn_board_info.sw_mcu.application_version.ci)
                                {
                                    // incoming ci version is greater or equal to the current one. Do FW update
                                    if (print_all_frame_info)
                                    {
                                        ESP_LOGI(TAG, "Incoming FW version is same/ newer than the Current FW version");
                                    }
                                }
                                else
                                {
                                    // incoming ci version is less than the current one. Don't FW update
                                    ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                    OTA_overall_status = false;
                                }
                            }
                            else
                            { // incoming ci version is greater than the current one. Do FW update
                                if (ci_version > cn_board_info.sw_mcu.application_version.ci)
                                {
                                    // incoming ci version is greater or equal to the current one. Do FW update
                                    if (print_all_frame_info)
                                    {
                                        ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                                    }
                                }
                                else
                                { // incoming ci version is equal or less than the current one. Don't FW update
                                    if (ci_version == cn_board_info.sw_mcu.application_version.ci)
                                    {
                                        ESP_LOGE(TAG, "Incoming FW version is same as the Current FW version");
                                    }
                                    else
                                    {
                                        ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                    }
                                    OTA_overall_status = false;
                                }
                            }
                        }
                        else
                        {
                            //incoming minor version is less than the current one. Don't FW update
                            ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                            OTA_overall_status = false;
                        }
                    }
                }
                else
                {
                    //incoming major version is less than the current one. Don't FW update
                    ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                    OTA_overall_status = false;
                }
            }
        }
        else if (strcmp(target_MCU, "cense_config") == 0)
        {
            target_MCU_int = CNGW_FIRMWARE_BINARY_TYPE_config;
        }
        if(!OTA_overall_status)
        {   //the FW is of older version. return with status false
            return OTA_overall_status;
        }
#ifdef GATEWAY_IPNODE

        LED_assign_task(CNGW_LED_CMD_FW_UPDATE, CNGW_LED_COMM); 

        esp_err_t result = NODE_Mainboard_OTA_Update_Data_Length_To_Be_Expected(total_expected_data, target_MCU_int, major_version, minor_version, ci_version, branch_id);
        if (result == ESP_OK)
        {
            OTA_overall_status = true;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to update OTA length or erase the required partitions");
            OTA_overall_status = false;
        }
#endif
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
        esp_err_t result = GW_ROOT_Mainboard_OTA_Update_Variables(total_expected_data, target_MCU_int, major_version, minor_version, ci_version, branch_id);
        if (result == ESP_OK)
        {
            OTA_overall_status = true;
            LED_assign_task(CNGW_LED_CMD_FW_UPDATE, CNGW_LED_CN);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to update OTA length or erase the required partitions");
            OTA_overall_status = false;
        }

#endif

    }
    else
    {
        ESP_LOGE(TAG, "Failed to separate name and version");
        OTA_overall_status = false;
    }
    return OTA_overall_status;
}

/**
 * @brief Get the Raw data from the ROOT. Checks if it is OTA related. If so, prepares the packet to be saved in NODE memory
 * @param value[in] Raw data from the ROOT
 * @return true if the Raw data is OTA related. false if not
 */
bool GW_Process_OTA_Command_Data(const char *value)
{
    if (!OTA_data_expected)
    {
        return false;
    }
    if (ota_agent_core_OTA_in_progress)
    {
        return false;
    }
    mupgrade_packet_t received_data = {0};
    memcpy(&received_data, value, sizeof(mupgrade_packet_t));
    if (received_data.type != MUPGRADE_TYPE_DATA)
    {
        return false;
    }
    if (!OTA_overall_status)
    {
        // previously there were errors detected. Therefore stop anymore furthur processing and return true (since it is OTA related information)
        return true;
    }
    if (next_expected_sequence_number != received_data.seq)
    {
        // the incoming FW is out of sequence!
        ESP_LOGE(TAG, "The incoming FW is out of sequence! expected seq: %d, what I got: %d", next_expected_sequence_number, received_data.seq);
        OTA_overall_status = false;
        return true;
    }
    
    total_received_data += received_data.size;
    next_expected_sequence_number++;
#ifdef GATEWAY_IPNODE
    esp_err_t result = NODE_Mainboard_OTA_Process_Data(received_data.data, received_data.size);
    if (result != ESP_OK)
    {
        // the FW was not successfully copied to the NODE memory. Therefore set the overall OTA status to false
        OTA_overall_status = false;
    }
#endif
    // whatever the result is above, return true since the data is OTA related
    return true;
}

/**
 * @brief Sets the OTA_Data_Expected to false. Checks if the current packet sequence is the expected total amount of packets. Checks for total data size integrity. If all is well, begin OTA to Mainboard.
 * @param structNodeReceived[in] JSON data from the ROOT
 * @return true if all criteria is met. false if not. The response is given when the OTA to mainboard is completed. (blocking function)
 */
bool GW_Process_OTA_Command_End(NodeStruct_t *structNodeReceived)
{
    bool state = false;
    OTA_data_expected = false;
    if(ota_agent_core_OTA_in_progress)
    {
        return false;
    }
    ESP_LOGI(TAG, "GW_Process_OTA_Command_End Function Called");
    // Check for total packet count integrity
    if (next_expected_sequence_number == (int)structNodeReceived->dValue)
    {
        ESP_LOGI(TAG, "Total received packet count matches the expected packet count. FW packet count verified.");
        state = true;
    }
    else
    {
        ESP_LOGE(TAG, "Packet count mismatch in received packets and the expected total packet count. FW update failed.");
        state = false;
    }
    if (state)
    {
        // Check for total data size integrity
        if (total_expected_data == total_received_data)
        {
            ESP_LOGI(TAG, "Total received data size matches the expected total size. FW size verified. Begin sending FW to Mainboard");
            // all checks OK. Start FW update to the Mainboard.
            // if the OTA fails when FW update is between NODE and Mainboard, it is captured here
            LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM); 
            LED_assign_task(CNGW_LED_CMD_FW_UPDATE, CNGW_LED_CN);
#ifdef GATEWAY_IPNODE
            state = NODE_do_Mainboard_OTA();
#endif
            LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
            if (!state)
            {
                LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_CN, LED_CHANGE_EXTENDED_DURATION);
            }
            else 
            {
                //set ota_agent_core_OTA_in_progress as true again so that Gw will not send power failure error when the mainboard restarts
                //ota_agent_core_OTA_in_progress = true;
                //free_SPI(target_MCU_int);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Size mismatch in received data and the expected total size. FW update failed.");
            LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
            LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_CN, LED_CHANGE_EXTENDED_DURATION);

            state = false;
        }
    }
    return state;
}

#endif