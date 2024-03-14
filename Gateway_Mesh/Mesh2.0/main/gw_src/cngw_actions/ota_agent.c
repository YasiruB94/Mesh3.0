/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: ota_agent.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: link between main GW code and the ota_agent_core.c
 * includes code for GATEWAY_ETHERNET, GATEWAY_SIM7080
 * 
 ******************************************************************************
 * The code for IP_NODE is removed since the GW is only of type ROOT now.
 * To get the code intended for GATEWAY_IPNODE, refer the branch CF-70-ETHERNET
 ******************************************************************************
 */


#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/ota_agent.h"

static const char *TAG = "ota_agent";
bool restart_ROOT_after_OTA = false; // if true, the ROOT restarts after every successful OTA session

#ifdef GATEWAY_DEBUGGING
static bool print_all_frame_info        = true;
static bool allow_same_FW_to_be_flashed = true;
#else
static bool print_all_frame_info        = false;
static bool allow_same_FW_to_be_flashed = false;
#endif


/**
 * @brief ROOT updates it's global OTA variables
 * @param data[in]          size of the total FW 
 * @param target_MCU[in]    STM32 MCU target type (CN, SW or DR)
 * @param major_version[in] incoming FW major_version
 * @param minor_version[in] incoming FW minor_version
 * @param ci_version[in]    incoming FW ci_version
 * @param branch_id[in]     incoming FW branch_id
 * @return ESP_OK if process is successful
 */
esp_err_t GW_ROOT_Mainboard_OTA_Update_Variables(size_t data, CNGW_Firmware_Binary_Type target_MCU, uint8_t major_version, uint8_t minor_version, uint8_t ci_version, uint8_t branch_id)
{
    // 1. check if an OTA is already in progress
    if (ota_agent_core_OTA_in_progress)
    {
        ESP_LOGE(TAG, "An OTA update is already in progress...");
        return ESP_FAIL;
    }

    // 2. Update global variables with required values
    ota_agent_core_data_length             = data;
    ota_agent_core_total_received_data_len = 0;
    ota_agent_core_target_MCU              = target_MCU;
    ota_agent_core_major_version           = major_version;
    ota_agent_core_minor_version           = minor_version;
    ota_agent_core_CI_version              = ci_version;
    ota_agent_core_branch_ID               = branch_id;

    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "Incoming FW target: %d, Version: %d.%d.%d-%d", target_MCU, major_version, minor_version, ci_version, branch_id);
    }

    return ESP_OK;
}

esp_err_t GW_ROOT_Check_ESP32_FW_version(char *URL)
{
    esp_err_t status = ESP_FAIL;
    char *start = strstr(URL, "cense_");
    char *end = strchr(start, 'b');

    if (start != NULL && end != NULL)
    {
        // find out the type of the target MCU and it's version
        char extractedString[50];
        char target_MCU[50];
        int major_version = 0;
        int minor_version = 0;
        int ci_version = 0;
        int branch_id = 0;
        strncpy(extractedString, start, end - start);
        extractedString[end - start] = '\0';
        ESP_LOGW(TAG, "FW binary name: %sbin", extractedString);

        if (sscanf(extractedString, "%[^-]-v%d.%d.%d.%d.", target_MCU, &major_version, &minor_version, &ci_version, &branch_id) == 5)
        {
            if (strcmp(target_MCU, "cense_gw_mcu") == 0)
            {
                ESP_LOGW(TAG, "Current FW version: %d.%d.%d-%d", cn_board_info.gw_mcu.application_version.major, cn_board_info.gw_mcu.application_version.minor, cn_board_info.gw_mcu.application_version.ci, cn_board_info.gw_mcu.application_version.branch_id);
                ESP_LOGW(TAG, "Incoming FW version: %d.%d.%d-%d", major_version, minor_version, ci_version, branch_id);

                // begin checking the branch ID. do teh FW update ONLY if the branch IDs match
                if (branch_id == cn_board_info.gw_mcu.application_version.branch_id)
                {
                    // begin comparing the major, minor and ci versions
                    if (major_version > cn_board_info.gw_mcu.application_version.major)
                    {
                        // incoming major version is greater than the current one. Do FW update
                        if (print_all_frame_info)
                        {
                            ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                        }
                        status = ESP_OK;
                    }
                    else
                    {
                        if (major_version == cn_board_info.gw_mcu.application_version.major)
                        {
                            // incoming major version is equal to the current one. check furthur
                            if (minor_version > cn_board_info.gw_mcu.application_version.minor)
                            {
                                // incoming minor version is greater than the current one. Do FW update
                                if (print_all_frame_info)
                                {
                                    ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                                }
                                status = ESP_OK;
                            }
                            else
                            {
                                if (minor_version == cn_board_info.gw_mcu.application_version.minor)
                                {
                                    // incoming minor version is equal to the current one. check furthur
                                    if (allow_same_FW_to_be_flashed)
                                    {
                                        if (ci_version >= cn_board_info.gw_mcu.application_version.ci)
                                        {
                                            // incoming ci version is greater or equal to the current one. Do FW update
                                            if (print_all_frame_info)
                                            {
                                                ESP_LOGI(TAG, "Incoming FW version is same/ newer than the Current FW version");
                                            }
                                            status = ESP_OK;
                                        }
                                        else
                                        {
                                            // incoming ci version is less than the current one. Don't FW update
                                            ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                        }
                                    }
                                    else
                                    { // incoming ci version is greater than the current one. Do FW update
                                        if (ci_version > cn_board_info.gw_mcu.application_version.ci)
                                        {
                                            // incoming ci version is greater than the current one. Do FW update
                                            if (print_all_frame_info)
                                            {
                                                ESP_LOGI(TAG, "Incoming FW version is newer than the Current FW version");
                                            }
                                            status = ESP_OK;
                                        }
                                        else
                                        { // incoming ci version is equal or less than the current one. Don't FW update
                                            if (ci_version == cn_board_info.gw_mcu.application_version.ci)
                                            {
                                                ESP_LOGE(TAG, "Incoming FW version is same as the Current FW version");
                                            }
                                            else
                                            {
                                                ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    // incoming minor version is less than the current one. Don't FW update
                                    ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                                }
                            }
                        }
                        else
                        {
                            // incoming major version is less than the current one. Don't FW update
                            ESP_LOGE(TAG, "Incoming FW version is older than the Current FW version");
                        }
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "Branch ID mismatch");
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to separate name");
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to separate name and version");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to separate name and version");
    }

    return status;
}

#ifdef GATEWAY_ETHERNET
/**
 * @brief OTA FW update intended for ESP32 ROOT+GATEWAY_ETHERNET device
 * @return ESP_OK if the OTA successful. ESP_FAIL if not
 */
bool GW_ROOT_UpgradeRootESP32Firmware(MeshStruct_t *structRootWrite)
{
    esp_err_t status = ESP_FAIL;
    char *error_string = "none";
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (strlen(structRootWrite->cReceivedData) >= 5)
    {
        status = GW_ROOT_Check_ESP32_FW_version(structRootWrite->cReceivedData);
        if (status == ESP_OK)
        {
            LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
            LED_assign_task(CNGW_LED_CMD_FW_UPDATE, CNGW_LED_GEN);
            Send_GW_message_to_AWS(64, 0, "Begin OTA");

            esp_http_client_config_t config =
                {
                    .url = (char *)structRootWrite->cReceivedData,
                    .cert_pem = NULL,
                    .event_handler = RootUtilities_httpEventHandler,
                };
            status = esp_https_ota(&config);
            if (status != ESP_OK)
            {
                error_string = "Firmware upgrade failed";
                status = ESP_FAIL;
            }
        }
        else
        {
            error_string = "Receiving FW is of an older version or is incompatible";
        }
    }
    else
    {
        error_string = "Invalid URL provided, skipping parsing";
    }

    if (status == ESP_FAIL)
    {
        Send_GW_message_to_AWS(65, 0, "End OTA");
        ESP_LOGE(TAG, "%s", error_string);
        Send_GW_message_to_AWS(65, 0, error_string);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_GEN);
        LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_COMM, LED_CHANGE_EXTENDED_DURATION);
        return false;
    }
    else
    {
        Send_GW_message_to_AWS(64, 0, "End OTA");
        ESP_LOGI(TAG, "Firmware Upgrade success");
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_GEN);
        RootUtilities_SendStatusUpdate(deviceMACStr, DEVICE_STATUS_RESTARTING);
        delayed_ESP_Restart(2000);

        return true;
    }
}
#endif

/**
 * @brief Build the struct to pass to OTA_Send_Binary
 * @return ESP_OK if the OTA to mainboard was successful. ESP_FAIL if not (blocking function)
 */
bool GW_ROOT_do_Mainboard_OTA()
{
    // 1. define and populate struct which is used for FW update to CN
    Binary_Data_Pkg_Info_t binary_file      = {0};
    binary_file.binary_size                 = ota_agent_core_data_length;
    if(ota_agent_core_target_MCU == CNGW_FIRMWARE_BINARY_TYPE_config)
    {
        binary_file.binary_size_mod             = ota_agent_core_data_length;
    }
    else

    {
       binary_file.binary_size_mod             = ota_agent_core_data_length + 150;
    }
    binary_file.initial_ptr                 = (const uint8_t *)ota_agent_core_target_start_address;
    binary_file.dist_release_ver.major      = 2;
    binary_file.dist_release_ver.minor      = 5;
    binary_file.dist_release_ver.ci         = 19;
    binary_file.dist_release_ver.branch_id  = 0;
    binary_file.binary_type                 = ota_agent_core_target_MCU;
    binary_file.binary_ver.major            = ota_agent_core_major_version;
    binary_file.binary_ver.minor            = ota_agent_core_minor_version;
    binary_file.binary_ver.ci               = ota_agent_core_CI_version;
    binary_file.binary_ver.branch_id        = ota_agent_core_branch_ID;
    // this is set as 0 for now. main reason: the sw bootloader determines if the FW data is encrypted or not depending on this value.
    // if it is 0, sw bootloader will not attempt to decode FW.
    // once we know how to encrypt the FW, we can add the proper CRC value here, which the target MCUs will check as a final validation. 
    if (ota_agent_core_target_MCU != CNGW_FIRMWARE_BINARY_TYPE_sw_mcu)
    {
        binary_file.binary_full_crc = calculate_total_crc(binary_file.initial_ptr , binary_file.binary_size);
    }
    else
    {
        binary_file.binary_full_crc = 0;
    }

    // 2. do OTA to CN routine. this will take the most time in the OTA process
    ota_agent_core_OTA_in_progress = true;
    GW_STATUS result = OTA_Send_Binary(binary_file);
    ota_agent_core_OTA_in_progress = false;

    // 3. get the OTA result and return it
    if (result == 32)
    {
        // The entire FW is properly copied to CN. Now check if the CN responded with CNGW_OTA_STATUS_Success, indicating it has accepted the new FW and is going to restart
        if (ota_agent_core_OTA_FW_accepted_by_CN)
        {
            return true;
        }
        else
        {
            if(print_all_frame_info)
            {
                ESP_LOGW(TAG, "CN did not respond with a proper FW OTA finishing. proceeding nevertheless..");
            }

            return true;
        }
    }
    else
    {
        return false;
    }

    return false;
}


/**
 * @brief Initialize OTA update process
 *  Determines the unused OTA partition to temporarily store OTA data
 *  Updates global "ota_agent_core_update_partition" variable to be used by other functions
 * @return ESP_OK if OTA partition found, else ESP_FAIL
 */
esp_err_t GW_ROOT_find_OTA_partition()
{
    const esp_partition_t *active_partition = esp_ota_get_boot_partition();
    if (strcmp(active_partition->label, "ota_0") == 0)
    {
        if (print_all_frame_info)
        {
            ESP_LOGI(TAG, "Current active partition is ota_0. Preparing partition ota_1 to store incoming firmware...");
        }
        ota_agent_core_update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    }
    else
    {
        if (print_all_frame_info)
        {
            ESP_LOGI(TAG, "Current active partition is factory OR ota_1. Preparing partition ota_0 to store incoming firmware...");
        }
        ota_agent_core_update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    }
    if (ota_agent_core_update_partition == NULL)
    {
        return ESP_FAIL;
    }
    ota_agent_core_target_start_address = ota_agent_core_update_partition->address;
    return ESP_OK;
}


/**
 * @brief Write firmware data to OTA partition
 *  ROOT writes incoming packets to the correct temporary memory location
 * @param data[in] pointer to FW bytes
 * @param size[in] length of the FW bytes
 * @return ESP_OK if data is successfully copied. ESP_FAIL if not
 */
esp_err_t GW_ROOT_write_OTA_packet(const uint8_t *data, size_t size)
{
    // 1. calculate the proper target address
    uint32_t val = ota_agent_core_target_start_address + ota_agent_core_total_received_data_len;

    // 2. write the packet to the target address
    esp_err_t ret1 = spi_flash_write(val, data, size);
    if (ret1 != ESP_OK)
    {
        return ESP_FAIL;
    }
    // 3. read back the copied data and cross check with the packet data to verify proper writing of data
    void *read_buffer = malloc(size);
    if (read_buffer == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory");
        goto safe_return;
    }
    else
    {
        esp_err_t err = spi_flash_read(val, read_buffer, size);
        if (err == ESP_OK)
        {
            // Compare the read data with the original data
            if (memcmp(data, read_buffer, size) != 0)
            {
                ESP_LOGE(TAG, "Data verification failed. The data does not match.");
                goto safe_return;
            }
        }
        else
        {
            // Handle read error
            ESP_LOGE(TAG, "SPI Failed to read: %s", esp_err_to_name(err));
            goto safe_return;
        }
    }

    // 4. free resources and exit function
    free(read_buffer);
    return ESP_OK;
safe_return:
    free(read_buffer);
    return ESP_FAIL;
}

/**
 * @brief ROOT begins the OTA FW update process
 *  Finds and clear out the memory space required to hold the FW temporarily
 * @param data[in] size of the total FW 
 * @return ESP_OK if memory is successfully cleared. ESP_FAIL if not
 */
esp_err_t GW_ROOT_prepare_memory_to_store_OTA_data_temporarily(size_t data)
{
    // 1. get the memory address to begin erasing 
    esp_err_t result = GW_ROOT_find_OTA_partition();
    if (result != ESP_OK)
    {
        return ESP_FAIL;
    }

    // 2. Update global variables with required values
    ota_agent_core_data_length = data;
    ota_agent_core_total_received_data_len = 0;

    // 3. begin erasing the required sectors
    // 3.1. calculate the number of sectors required for the erase operation
    size_t sector_size = 4096;
    size_t sectors_to_erase = (data + sector_size - 1) / sector_size;
    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "OTA data about to receive. total firmware size: %d, Firmware sector address: 0x%08x, number of sectors to erase: %d", ota_agent_core_data_length, ota_agent_core_target_start_address, sectors_to_erase);
    }

    // 3.2. erase the specified memory range sector by sector
    for (size_t i = 0; i < sectors_to_erase; i++)
    {
        uint32_t sector_address = ota_agent_core_target_start_address + (i * sector_size);
        esp_err_t err = spi_flash_erase_range(sector_address, sector_size);
        if (err == ESP_OK)
        {
            if (print_all_frame_info)
            {
                ESP_LOGI(TAG, "\tSector at address\t0x%08x erased...", sector_address);
            }
        }
        else
        {
            ESP_LOGE(TAG, "\tFailed to erase sector at address\t0x%08x: %s", sector_address, esp_err_to_name(err));
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

/**
 * @brief ROOT processes an OTA FW packet
 *  Finds and writes the packet to previously cleared memory
 * @param data[in] pointer to the FW packet
 * * @param data[in] packet size
 * @return ESP_OK if packet is successfully copied
 */
esp_err_t GW_ROOT_process_OTA_packet(uint8_t *data, size_t data_len)
{
    // 1. write the OTA data
    esp_err_t result = GW_ROOT_write_OTA_packet(data, data_len);

    // 2. update global variables to track progress
    ota_agent_core_target_address = ota_agent_core_target_start_address + ota_agent_core_total_received_data_len;
    ota_agent_core_total_received_data_len += data_len;
    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "\tData len: %d,\ttotal_received: %d,\ttarget address: 0x%08x,\twriting data status: %s", data_len, ota_agent_core_total_received_data_len, ota_agent_core_target_address, esp_err_to_name(result));
    }

    if (ota_agent_core_total_received_data_len == ota_agent_core_data_length)
    {
        if (print_all_frame_info)
        {
            ESP_LOGI(TAG, "All data received");
        }
    }
    return result;
}

/**
 * @brief helper function to return the OTA target start address static variable to other functions
 * @return ota_agent_core_target_start_address
 */
uint32_t GW_ROOT_get_OTA_start_address()
{
    return ota_agent_core_target_start_address;
}

#endif


