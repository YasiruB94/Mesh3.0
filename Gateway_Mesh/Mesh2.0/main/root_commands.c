#ifdef ROOT
#include "includes/root_commands.h"
#include "includes/aws.h"
#include "../gw_includes/SIM7080.h"
#include "../gw_includes/ota_agent.h"
static const char *TAG = "RootCmnds";

// The functions return a 1 for success, 0 for fail and a 2 for not applicable
uint8_t PublishSensorData(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "PublishSensorData Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(sensorLogTopic, structRootReceived->cptrString);
    return 2;
}

uint8_t PublishRegisterDevice(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "PublishRegisterDevice Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(registerLogTopic, structRootReceived->cptrString);
    return 2;
}

uint8_t PublishNodeControlSuccess(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "PublishNodeControlSuccess Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(controlLogTopicSuccess, structRootReceived->cptrString);
    return 2;
}

uint8_t PublishNodeControlFail(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "PublishNodeControlFail Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(controlLogTopicFail, structRootReceived->cptrString);
    return 2;
}

uint8_t RestartRoot(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "RestartRoot Function Called");
    if (RootUtilities_SendStatusUpdate(deviceMACStr, DEVICE_STATUS_RESTARTING))
        ESP_LOGI(TAG, "Status sent!");
    else
        ESP_LOGI(TAG, "Status sending error!");
    vTaskDelay(3000 / portTICK_RATE_MS);
    esp_restart();
    // P.S. no value will be returned after esp_restart() is called. The line below will not run.
    return 2;
}

uint8_t TestRoot(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "TestRoot Function Called");
    if (RootUtilities_SendStatusUpdate(deviceMACStr, DEVICE_STATUS_ALIVE))
        return 1;
    else
        return 0;
}

// P.S. this function has a memory leak (coming from the ESP library)
uint8_t UpdateRootFW(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "UpdateRootFW Function Called");
#ifdef GATEWAY_ETHERNET
    // the ROOT is a ROOT+GATEWAY_ETHERNET device.
        MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
        structRootWrite->ubyNodeMac = (uint8_t *)malloc((structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
        memcpy(structRootWrite->ubyNodeMac, structRootReceived->ubyptrMacs, (structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
        structRootWrite->cReceivedData = (char *)malloc((strlen(structRootReceived->cptrString) + 1) * sizeof(char));
        strcpy(structRootWrite->cReceivedData, structRootReceived->cptrString);
        structRootWrite->ubyNumOfNodes = (uint8_t)structRootReceived->uwValue;
        bool result = GW_ROOT_UpgradeRootESP32Firmware(structRootWrite);
        free(structRootWrite->cReceivedData);
        free(structRootWrite->ubyNodeMac);
        free(structRootWrite);
    return result;
#else
#ifdef GATEWAY_SIM7080
    // the ROOT is a ROOT+GATEWAY_SIM7080 device. The FW must be downloaded using the SIM7080 module
    if (strlen(structRootReceived->cptrString) >= 5)
    {
        MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
        structRootWrite->ubyNodeMac = (uint8_t *)malloc((structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
        memcpy(structRootWrite->ubyNodeMac, structRootReceived->ubyptrMacs, (structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
        structRootWrite->cReceivedData = (char *)malloc((strlen(structRootReceived->cptrString) + 1) * sizeof(char));
        strcpy(structRootWrite->cReceivedData, structRootReceived->cptrString);
        structRootWrite->ubyNumOfNodes = (uint8_t)structRootReceived->uwValue;

        bool result = SIM7080_UpgradeRootESP32Firmware(structRootWrite);
        free(structRootWrite->cReceivedData);
        free(structRootWrite->ubyNodeMac);
        free(structRootWrite);
        return result;
    }
#else
    // the ROOT is a normal ROOT.
    if (strlen(structRootReceived->cptrString) >= 5)
    {
        esp_http_client_config_t config = {
            .url = (char *)structRootReceived->cptrString,
            .cert_pem = NULL,
            .event_handler = RootUtilities_httpEventHandler,
        };
        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Firmware Upgrade success");
            RootUtilities_SendStatusUpdate(deviceMACStr, DEVICE_STATUS_RESTARTING);
            vTaskDelay(3000 / portTICK_RATE_MS);
            esp_restart();
            // P.S. no value will be returned after esp_restart() is called. The lines below will not run.
        }
        else
        {
            ESP_LOGE(TAG, "Firmware upgrade failed");
            return 0;
        }
    }

#endif
#endif
    return 0;
}
// Need to incorporate into the new flow
uint8_t UpdateFW(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "UpdateFW Function Called");
    if (structRootReceived->ubyptrMacs == NULL || strlen(structRootReceived->cptrString) <= 5)
        return 0;
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, structRootReceived->ubyptrMacs, (structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->cReceivedData = (char *)malloc((strlen(structRootReceived->cptrString) + 1) * sizeof(char));
    strcpy(structRootWrite->cReceivedData, structRootReceived->cptrString);
    structRootWrite->ubyNumOfNodes = (uint8_t)structRootReceived->uwValue;
    // ESP_LOGI(TAG, "This is the string %s", structRootWrite->cReceivedData);
    bool success = false;
    if (RootUtilities_NodeAddressVerification(structRootWrite))
    {
        if (structRootReceived->ubyCommand == enumRootCmndKey_UpdateNodeFW)
        {
            if (RootUtilities_UpgradeFirmware(structRootWrite, true))
            {
                success = true;
            }
        }
        else
        {
            // sensor update command, no restart required
            if (RootUtilities_UpgradeFirmware(structRootWrite, false))
            {
                success = true;
            }
        }
    }
    free(structRootWrite->cReceivedData);
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (success)
        return 1;
    else
        return 0;
}

/**
 * @brief handles ALL Cence FW + Config. The target can be a NODE or a GATEWAY_ETHERNET ROOT
 * When the command is sent from AWS, if the "val" > 0, it is intended for a NODE. 
 * If the "val" == 0, it is intended for GATEWAY_ETHERNET ROOT
 * @return ESP_OK if OTA partition found, else ESP_FAIL
 */
uint8_t UpdateCenceFW(RootStruct_t *structRootReceived)
{
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)

    ESP_LOGI(TAG, "UpdateCenceFW Function Called");
    if ((uint8_t)structRootReceived->uwValue == 0)
    {
        // intended target is the ROOT itself. we do not expect a value to structRootReceived->ubyptrMacs
        if (strlen(structRootReceived->cptrString) <= 5)
        {
            return 0;
        }
    }
    else
    {
        // intended target is a NODE. we expect values for structRootReceived->ubyptrMacs
        if (structRootReceived->ubyptrMacs == NULL || strlen(structRootReceived->cptrString) <= 5)
        {
            return 0;
        }
    }

    bool success = false;
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, structRootReceived->ubyptrMacs, (structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->cReceivedData = (char *)malloc((strlen(structRootReceived->cptrString) + 1) * sizeof(char));
    strcpy(structRootWrite->cReceivedData, structRootReceived->cptrString);
    structRootWrite->ubyNumOfNodes = (uint8_t)structRootReceived->uwValue;

    // check if the URL carries "cense" letters (an additional protection layer)
    char *strcompare = strstr(structRootWrite->cReceivedData, "cense_");
    if (strcompare != NULL)
    {  
        // URL is intended for cense FW/config
        // check the validity of the target
        if (structRootWrite->ubyNumOfNodes > 0 && structRootReceived->ubyptrMacs != NULL)
        {
            // intended target is a NODE(S). Verify the NODE address exists and proceed with FW update
            if (RootUtilities_NodeAddressVerification(structRootWrite))
            {
#ifdef GATEWAY_ETHERNET
                if (RootUtilities_UpgradeETHNodeCenceFirmware(structRootWrite))
                {
                    success = true;
                }
#endif
#ifdef GATEWAY_SIM7080
                if (SIM7080_UpgradeNodeCenceFirmware(structRootWrite))
                {
                    success = true;
                }
#endif
            }
        }

#ifdef GATEWAY_ETHERNET
        else if (structRootWrite->ubyNumOfNodes == 0)
        {
            // intended target is the ROOT itself (ROOT must be GATEWAY_ETHERNET type)
            if (RootUtilities_UpgradeETHRootCenceFirmware(structRootWrite))
            {
                success = true;
            }
        }
#else
#ifdef GATEWAY_SIM7080
        else if (structRootWrite->ubyNumOfNodes == 0)
        {
            // intended target is the ROOT itself (ROOT must be GATEWAY_SIM7080 type)
            if (SIM7080_UpgradeRootCenceFirmware(structRootWrite))
            {
                success = true;
            }
        }
#endif
#endif
    }

    free(structRootWrite->cReceivedData);
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (success)
        return 1;
    else
        return 0;
        
#else

    ESP_LOGE(TAG, "UpdateCenceFW Function not applicable");
    return 0;

#endif
}

uint8_t CreateGroup(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "CreateGroup Function Called");

    if (structRootReceived->ubyptrMacs == NULL)
        return 0;
    structRootReceived->ubyCommand = 3; // node command for CreateGroup
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, structRootReceived->ubyptrMacs, (structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->ubyNumOfNodes = (uint8_t)structRootReceived->uwValue;
    bool verified = RootUtilities_NodeAddressVerification(structRootWrite);
    if (verified)
    {
        if (!RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWrite, false))
            return 0;
    }
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (verified)
        return 1;
    else
        return 0;
}

uint8_t DeleteGroup(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "DeleteGroup Function Called");
    if (structRootReceived->ubyptrMacs == NULL)
        return 0;
    structRootReceived->ubyCommand = 4; // node command for DeleteGroup
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, structRootReceived->ubyptrMacs, (structRootReceived->uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->ubyNumOfNodes = (uint8_t)structRootReceived->uwValue;
    bool verified = RootUtilities_NodeAddressVerification(structRootWrite);
    if (verified)
        // if (!RootUtilities_PrepareJsonAndSend(4, 0, structRootReceived->cptrString, structRootWrite, structRootReceived->usrID))
        if (!RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWrite, false))
            return 0;
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (verified)
        return 1;
    else
        return 0;
}

uint8_t TestGroup(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "TestGroup Function Called");
    if (strlen(structRootReceived->cptrString) > 2 || strlen(structRootReceived->cptrString) <= 1)
        return 0;                       // Making sure that the last 2 digits of the group ID are not bigger than double digits
    structRootReceived->ubyCommand = 1; // command for TestNode
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((MWIFI_ADDR_LEN) * sizeof(uint8_t));
    char cptrGroupMacStr[18] = "01:00:5e:ae:ae:";
    strcat(cptrGroupMacStr, structRootReceived->cptrString);
    if (!Utilities_StringToMac(cptrGroupMacStr, structRootWrite->ubyNodeMac))
        return 0;
    structRootWrite->ubyNumOfNodes = 1; // Sending to one group only
    RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWrite, true);
    // RootUtilities_PrepareJsonAndSendDataToGroup(1, 0, cptrNullString, structRootWrite, structRootReceived->usrID);
    // free(structRootWrite->cReceivedData);
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    return 1;
}

uint8_t SendGroupCmd(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "SendGroupCmd Function Called");
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((MWIFI_ADDR_LEN) * sizeof(uint8_t));
    char cptrGroupMacStr[18] = "01:00:5e:ae:ae:";
    bool exists = false;
    cJSON *json = cJSON_Parse(structRootReceived->cptrString);
    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            ESP_LOGE(TAG, "Error while parsing");
            cJSON_Delete(json);
            return 0;
        }
    }
    else
    {
        cJSON *cjsonGroupId = cJSON_GetObjectItemCaseSensitive(json, "grpID");
        if (cJSON_IsString(cjsonGroupId) && (cjsonGroupId->valuestring != NULL) && Utilities_ValidateHexString(cjsonGroupId->valuestring, 2))
        {
            exists = true;
            strcat(cptrGroupMacStr, cjsonGroupId->valuestring);
            if (!Utilities_StringToMac(cptrGroupMacStr, structRootWrite->ubyNodeMac))
                return 0;
            structRootWrite->ubyNumOfNodes = 1; // Sending to one group only
            structRootWrite->cReceivedData = structRootReceived->cptrString;
            RootUtilities_SendDataToGroup(structRootWrite);
        }
    }
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    cJSON_Delete(json);
    if (exists)
        return 1;
    else
        return 0;
}

uint8_t AddOrgInfo(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "AddOrgInfo Function Called");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return 0;
    if (nvs_set_str(nvsHandle, "orgID", structRootReceived->cptrString) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return 0;
    }
    if (nvs_commit(nvsHandle) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return 0;
    }
    nvs_close(nvsHandle);
    strcpy(orgID, structRootReceived->cptrString);
    AWS_CreateTopics();
    return 1;
}

uint8_t MessageToAWS(RootStruct_t *structRootReceived)
{
    ESP_LOGI(TAG, "MessageToAWS Function Called");
    RootUtilities_SendDataToAWS(logTopic, structRootReceived->cptrString);
    return 2;
}

#endif