#ifdef ROOT
#include "Includes/root_operations.h"

static const char *TAG = "RootOperations";

void RootOperations_RootReadTask(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type = {0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0};

    MDF_LOGI("Root read task is running");

    for (;;)
    {
        if (!mwifi_is_started())
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_recv", mdf_err_to_name(ret));

        if (data_type.upgrade)
        { // This mesh package contains upgrade data.
            ret = mupgrade_root_handle(src_addr, data, size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_root_handle", mdf_err_to_name(ret));
        }
        else
        {
            char *cptrData = (char *)pvPortMalloc((size + 1) * sizeof(char));
            memset(cptrData, 0, size + 1);
            strcpy(cptrData, data);
            if (xQueueSendToBack(rootCommandQueue, &cptrData, (TickType_t)0) != pdPASS)
            {
                ESP_LOGE(TAG, "Queue is full");
                vPortFree(cptrData);
                //Report back to AWS here letting us know that the queue is full and to wait for the next command
                //The queue most likely will neveer fill up but you never know
            }
        }
    }

    MDF_LOGW("Root read task is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}

void RootOperations_RootGroupReadTask(void *arg) // This is a read task for root, only here so the root can recieve group messages.
{
    mdf_err_t ret = MDF_OK;
    char *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    for (;;)
    {
        if (!mwifi_is_connected() && !(mwifi_is_started() && esp_mesh_is_root()))
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, size);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));
        ESP_LOGI(TAG, "Node receive, data: << %.*s >>", size, data);
    }

    ESP_LOGW(TAG, "Root group read task exited");

    MDF_FREE(data);
    vTaskDelete(NULL);
}

void RootOperations_ProcessNodeCommands()
{
    char *payload = NULL;
    while (true)
    {
        if (nodeCommandQueue != NULL)
        {
            if (xQueueReceive(nodeCommandQueue, &payload, (TickType_t)0))
            {
                MeshStruct_t *structRootWrite = malloc(sizeof(MeshStruct_t));
                structRootWrite->ubyNodeMac = malloc(MWIFI_ADDR_LEN * sizeof(uint8_t));
                structRootWrite->cReceivedData = (char *)malloc((strlen(payload) + 1) * sizeof(char));
                memset(structRootWrite->cReceivedData, 0, strlen(payload));
                bool parsedAndSent = RootUtilities_ParseNodeAddressAndData(structRootWrite, payload);
                if (parsedAndSent)
                {
                    if (!RootUtilities_RootWrite(structRootWrite))
                        parsedAndSent = false;
                }
                if (!parsedAndSent)
                    RootUtilities_SendDataToAWS(controlLogTopicFail, payload);
                free(structRootWrite->cReceivedData);
                free(structRootWrite->ubyNodeMac);
                free(structRootWrite);
                vPortFree(payload);
                ESP_LOGI(TAG, "Stack for task '%s': %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); //Leave this for watchdog
    }
}

void RootOperations_ProcessRootCommands()
{
    char *payload = NULL;
    uint8_t *ubyptrNodeMacs = NULL;
    bool macsUsed = false;
    while (true)
    {
        if (rootCommandQueue != NULL)
        {
            if (xQueueReceive(rootCommandQueue, &payload, (TickType_t)0))
            {
                cJSON *json = cJSON_Parse(payload);

                if (json == NULL)
                {
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != NULL)
                    {
                        ESP_LOGE(TAG, "Error while parsing RootOperations_ProcessRootCommands");
                    }
                    RootUtilities_SendDataToAWS(controlLogTopicFail, payload);
                }
                else
                {
                    //heap_caps_check_integrity_all(true);
                    cJSON *cjCommand = cJSON_GetObjectItemCaseSensitive(json, "cmnd");
                    if (cJSON_IsNumber(cjCommand))
                    {
                        cJSON *cjValue = cJSON_GetObjectItemCaseSensitive(json, "val");
                        if (cJSON_IsNumber(cjValue))
                        {
                            cJSON *cjString = cJSON_GetObjectItemCaseSensitive(json, "str");
                            if (cJSON_IsString(cjString) && (cjString->valuestring != NULL))
                            {
                                cJSON *cjMacs = cJSON_GetObjectItemCaseSensitive(json, "macs");
                                if (cJSON_IsArray(cjMacs))
                                {
                                    if ((cjValue->valueint > 0) && (cJSON_GetArraySize(cjMacs) == cjValue->valueint * MWIFI_ADDR_LEN))
                                    {
                                        macsUsed = true;
                                        ubyptrNodeMacs = malloc(cjValue->valueint * MWIFI_ADDR_LEN * sizeof(uint8_t));
                                        ubyptrNodeMacs = RootUtilities_ParseMacAddress(cjMacs, ubyptrNodeMacs);
                                    }
                                }
                                uint8_t returnVal = RootUtilities_ValidateAndExecuteCommand((uint8_t)cjCommand->valueint, cjValue->valueint, cjString->valuestring, ubyptrNodeMacs);
                                if (returnVal == 1)
                                    RootUtilities_SendDataToAWS(controlLogTopicSuccess, payload);
                                else if (returnVal == 0)
                                    RootUtilities_SendDataToAWS(controlLogTopicFail, payload);
                                goto RootOperations_ProcessRootCommands_End;
                            }
                        }
                    }
                    RootUtilities_SendDataToAWS(controlLogTopicFail, payload);
                }
            RootOperations_ProcessRootCommands_End:
                heap_caps_check_integrity_all(true);
                cJSON_Delete(json);
                vPortFree(payload);
                if (macsUsed)
                {
                    macsUsed = false;
                    free(ubyptrNodeMacs);
                }
                ESP_LOGI(TAG, "Stack for task under RootOperations_ProcessRootCommands'%s': %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); //Leave this for watchdog
    }
}
#endif