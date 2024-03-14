#ifdef ROOT
#include "includes/root_commands.h"
#include "includes/aws.h"
static const char *TAG = "RootCmnds";

//The functions return a 1 for success, 0 for fail and a 2 for not applicable
uint8_t PublishSensorData(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "PublishSensorData Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(sensorLogTopic, cptrString);
    return 2;
}

uint8_t PublishControlData(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "PublishControlData Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(controlLogTopic, cptrString);
    return 2;
}

uint8_t PublishNodeControlSuccess(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "PublishNodeControlSuccess Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(controlLogTopicSuccess, cptrString);
    return 2;
}

uint8_t PublishNodeControlFail(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "PublishNodeControlFail Function Called");
    if (strcmp(orgID, "") == 0)
        return 2;
    RootUtilities_SendDataToAWS(controlLogTopicFail, cptrString);
    return 2;
}


uint8_t RestartRoot(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "RestartRoot Function Called");
    if (RootUtilities_SendStatusUpdate(deviceMACStr, 2))
        ESP_LOGI(TAG, "Status sent!");
    else
        ESP_LOGI(TAG, "Status sending error!");
    vTaskDelay(3000 / portTICK_RATE_MS);
    esp_restart();
    // P.S. no value will be returned after esp_restart() is called. The line below will not run.
    return 2;
}

uint8_t TestRoot(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "TestRoot Function Called");
    if (RootUtilities_SendStatusUpdate(deviceMACStr, 1))
        return 1;
    else
        return 0;
}

//Need to incorporate into the new flow
uint8_t UpdateRootFW(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "UpdateRootFW Function Called");
    if (strlen(cptrString) >= 5)
    {
        esp_http_client_config_t config = {
            .url = (char *)cptrString,
            .cert_pem = NULL,
            .event_handler = RootUtilities_httpEventHandler,
        };
        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Firmware Upgrade success");
            RootUtilities_SendStatusUpdate(deviceMACStr, 2);
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
    return 0;
}
//Need to incorporate into the new flow
uint8_t UpdateNodeFW(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "UpdateNodeFW Function Called");
    if (ubyptrMacs == NULL || strlen(cptrString) <= 5)
        return 0;
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, ubyptrMacs, (uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->cReceivedData = (char *)malloc((strlen(cptrString) + 1) * sizeof(char));
    strcpy(structRootWrite->cReceivedData, cptrString);
    structRootWrite->ubyNumOfNodes = (uint8_t)uwValue;
    // ESP_LOGI(TAG, "This is the string %s", structRootWrite->cReceivedData);
    bool addVerified = false;
    if (RootUtilities_NodeAddressVerification(structRootWrite))
    {
        addVerified = true;
        // GW required changes: filter out the ESP firmware from STM32 firmware
        char *strcompare = strstr(structRootWrite->cReceivedData, "cense");
        if (strcompare != NULL)
        {
            MDF_LOGW("Firmware downloaded is cense related");
            RootUtilities_UpgradeNodeCenceFirmware(structRootWrite);
        }
        else
        {
            MDF_LOGW("Firmware downloaded is ESP_MCU related");
            RootUtilities_UpgradeNodeFirmware(structRootWrite);
        }
    }
#ifdef GATEWAY_ETH
    else
    {
        char *strcompare = strstr(structRootWrite->cReceivedData, "cense");
        if (strcompare != NULL)
        {
            addVerified = true;
            MDF_LOGW("Firmware downloaded is cense related for ROOT");
            RootUtilities_ETHROOTUpgradeNodeCenceFirmware(structRootWrite);
        }
    }

#endif
    free(structRootWrite->cReceivedData);
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (addVerified)
        return 1;
    else
        return 0;
}

uint8_t CreateGroup(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "CreateGroup Function Called");
    if (ubyptrMacs == NULL)
        return 0;
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, ubyptrMacs, (uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->ubyNumOfNodes = (uint8_t)uwValue;
    bool verified = RootUtilities_NodeAddressVerification(structRootWrite);
    if (verified)
        if (!RootUtilities_PrepareJsonAndSend(3, 0, cptrString, structRootWrite))
            return 0;
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (verified)
        return 1;
    else
        return 0;
}

uint8_t DeleteGroup(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "DeleteGroup Function Called");
    if (ubyptrMacs == NULL)
        return 0;
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    memcpy(structRootWrite->ubyNodeMac, ubyptrMacs, (uwValue * MWIFI_ADDR_LEN) * sizeof(uint8_t));
    structRootWrite->ubyNumOfNodes = (uint8_t)uwValue;
    bool verified = RootUtilities_NodeAddressVerification(structRootWrite);
    if (verified)
        if (!RootUtilities_PrepareJsonAndSend(4, 0, cptrString, structRootWrite))
            return 0;
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    if (verified)
        return 1;
    else
        return 0;
}

uint8_t TestGroup(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "TestGroup Function Called");
    if (strlen(cptrString) > 2 || strlen(cptrString) <= 1)
        return 0; //Making sure that the last 2 digits of the group ID are not bigger than double digits
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((MWIFI_ADDR_LEN) * sizeof(uint8_t));
    char cptrGroupMacStr[18] = "01:00:5e:ae:ae:";
    strcat(cptrGroupMacStr, cptrString);
    if (!Utilities_StringToMac(cptrGroupMacStr, structRootWrite->ubyNodeMac))
        return 0;
    structRootWrite->ubyNumOfNodes = 1; // Sending to one group only
    char *cptrNullString = "";
    RootUtilities_PrepareJsonAndSendDataToGroup(1, 0, cptrNullString, structRootWrite);
    //free(structRootWrite->cReceivedData);
    free(structRootWrite->ubyNodeMac);
    free(structRootWrite);
    return 1;
}

uint8_t SendGroupCmd(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs)
{
    ESP_LOGI(TAG, "SendGroupCmd Function Called");
    MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
    structRootWrite->ubyNodeMac = (uint8_t *)malloc((MWIFI_ADDR_LEN) * sizeof(uint8_t));
    char cptrGroupMacStr[18] = "01:00:5e:ae:ae:";
    bool exists = false;
    cJSON *json = cJSON_Parse(cptrString);
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
            structRootWrite->cReceivedData = cJSON_PrintUnformatted(json);
            RootUtilities_SendDataToGroup(structRootWrite);
            free(structRootWrite->cReceivedData);
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

uint8_t AddOrgInfo(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs)
{
    ESP_LOGI(TAG, "AddOrgInfo Function Called");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return 0;
    if (nvs_set_str(nvsHandle, "orgID", cptrString) != ESP_OK)
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
    strcpy(orgID, cptrString);
    AWS_CreateTopics();
    return 1;
}

uint8_t MessageToAWS(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs)
{
    ESP_LOGI(TAG, "MessageToAWS Function Called");
    RootUtilities_SendDataToAWS(logTopic, cptrString);
    return 2;
}

#endif