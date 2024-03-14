#ifdef IPNODE
#include "includes/node_operations.h"

static const char *TAG = "NodeOperations";

void NodeOperations_CommandExecutionTask(void *arg)
{
    char *cptrNodeData = NULL;
    bool isArray = false;
    while (true)
    {
        if (nodeReadQueue != NULL)
        {
            if (xQueueReceive(nodeReadQueue, &cptrNodeData, (TickType_t)0))
            {
                NodeStruct_t *structNodeReceived = malloc(sizeof(NodeStruct_t));
                cJSON *json = cJSON_Parse(cptrNodeData);
                //GW required changes: OTA data does not come in the form of JSON. therefore the data is parsed to GW OTA function.
                //If the function does not expect OTA data, it returns immediately.
                bool OTA_data = GW_Process_OTA_Command_Data(cptrNodeData);
                if (json == NULL)
                {
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != NULL)
                    {
                        if (!OTA_data)
                        {
                            ESP_LOGE(TAG, "Error while parsing NodeOperations_CommandExecutionTask");
                        }
                    }
                    if (!OTA_data)
                    {
                        NodeUtilities_PrepareJsonAndSendToRoot(65, 0, cptrNodeData);
                    }
                }
                else
                {
                    // Parse data
                    cJSON *cjCommand = cJSON_GetObjectItemCaseSensitive(json, "cmnd");
                    if (cJSON_IsNumber(cjCommand))
                    {
                        structNodeReceived->ubyCommand = cjCommand->valueint;
                        cJSON *cjValue = cJSON_GetObjectItemCaseSensitive(json, "val");
                        if (cJSON_IsNumber(cjValue) || cJSON_IsArray(cjValue))
                        {
                            structNodeReceived->dValue = cjValue->valuedouble;
                            structNodeReceived->arrValueSize = 0;
                            if (cJSON_IsArray(cjValue))
                            {
                                isArray = true;
                                structNodeReceived->arrValueSize = cJSON_GetArraySize(cjValue);
                                structNodeReceived->arrValues = malloc(cJSON_GetArraySize(cjValue) * sizeof(uint8_t));
                                NodeUtilities_ParseArrVal(cjValue, structNodeReceived->arrValues);
                            }
                            cJSON *cjString = cJSON_GetObjectItemCaseSensitive(json, "str");
                            if (cJSON_IsString(cjString))
                            {
                                structNodeReceived->cptrString = cjString->valuestring;
                                bool returnVal = NodeUtilities_ValidateAndExecuteCommand(structNodeReceived);
                                if (returnVal)
                                {
                                    NodeUtilities_PrepareJsonAndSendToRoot(64, 0, cptrNodeData);
                                }
                                else
                                {
                                    NodeUtilities_PrepareJsonAndSendToRoot(65, 0, cptrNodeData);
                                }
                                goto NodeOperations_CommandExecutionTask_End;
                            }
                        }
                    }
                    // GW required changes: sometimes OTA data is mis-represented as JSON formatted, even though it is not.
                    // therefore, first check if the current data is OTA related, and if so, then dont send information to AWS
                    if (!OTA_data)
                    {
                        NodeUtilities_PrepareJsonAndSendToRoot(65, 0, cptrNodeData);
                    }
                }
            NodeOperations_CommandExecutionTask_End:
                cJSON_Delete(json);
                vPortFree(cptrNodeData);
                free(structNodeReceived);
                if (isArray)
                {
                    free(structNodeReceived->arrValues);
                    isArray = false;
                }
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); //Leave this for watchdog
    }
}

void NodeOperations_RootSendTask(void *arg)
{
    char *rootSendData = NULL;
    while (true)
    {
        if (rootSendQueue != NULL)
        {
            if (xQueueReceive(rootSendQueue, &rootSendData, (TickType_t)0))
            {
                mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_UNICAST, .compression = true};
                mdf_err_t ret = mwifi_write(NULL, &data_type, rootSendData, strlen(rootSendData) + 1, true);
                //TODO@sagar448 #16 handling error if the mwifi write function did not successfully send!
                if (ret == MDF_OK)
                {
                    ESP_LOGI(TAG, "Data Sent to Root");
                }
                else
                {
                    //Report back to a error topic saying could not send
                    ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
                }
                vPortFree(rootSendData);
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); //Leave this for watchdog
    }
}

void NodeOperations_NodeReadTask(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    MDF_LOGI("Node read task is running");
    for (;;)
    {
        if (!mwifi_is_connected() && !(mwifi_is_started() && esp_mesh_is_root()))
        {
            vTaskDelay(500 / portTICK_RATE_MS);
            //ESP_LOGI(TAG, "Here ");
            continue;
        }
        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_read, ret: %s", mdf_err_to_name(ret));
        if (data_type.upgrade)
        {
            MDF_LOGW("node receiving upgraded firmware");
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));
        }
        else
        {
            MDF_LOGI("Receive [NODE] addr: " MACSTR ", size: %d, data: %s",
                     MAC2STR(src_addr), size, data);
            char *cptrData = (char *)pvPortMalloc((size + 1) * sizeof(char));
            memcpy(cptrData, data, size);
            if (xQueueSendToBack(nodeReadQueue, &cptrData, (TickType_t)0) != pdPASS)
            {
                ESP_LOGE(TAG, "Queue is full");
                vPortFree(cptrData);
                //Report back to Root here letting us know that the queue is full and to wait for the next command
                //The queue most likely will neveer fill up but you never know
            }
            ESP_LOGI(TAG, "Stack for task '%s': %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
        }
    }
    MDF_LOGW("Note read task is exiting");
    MDF_FREE(data);
    vTaskDelete(NULL);
}

void NodeOperations_ProcessEspNowCommands()
{
    EspNowStruct *recv_cb;
    while (true)
    {
        if (espNowQueue != NULL)
        {
            if (xQueueReceive(espNowQueue, &recv_cb, (TickType_t)0))
            {
                Sensor_SendPendingMessage(recv_cb);
                size_t requiredSize = 0;
                if (Sensor_AlreadyExists(recv_cb, &requiredSize))
                {
                    if (strcmp((char *)recv_cb->data, "LFN") == 0)
                    {
                        Sensor_SendFriendInfo(recv_cb->mac_addr);
                    }
                    else if (strcmp((char *)recv_cb->data, "PRD") == 0)
                    {
                        //Sensor was paired successfully, need to send
                    }
                    else
                    {
                        uint8_t sensorAction = 0; //assuming here that it is sensor data
                        char macOfNodeStr[18] = "";
                        EspNow_ParseData(recv_cb, &requiredSize, &sensorAction, macOfNodeStr);
                        if (sensorAction == 0)
                        { //its not a control datapoint, its just a sensor data
                            NodeUtilities_PrepareJsonAndSendToRoot(58, 0, (char *)recv_cb->data);
                        }
                        else
                        {
                            cJSON *json = cJSON_Parse((char *)recv_cb->data);
                            if (json == NULL)
                            {
                                const char *error_ptr = cJSON_GetErrorPtr();
                                if (error_ptr != NULL)
                                {
                                    ESP_LOGE(TAG, "Error while parsing %s", error_ptr);
                                }
                            }
                            else
                            {
                                cJSON_AddItemToObject(json, "macOfNode", cJSON_CreateString(macOfNodeStr));
                                char *dataToSend = cJSON_PrintUnformatted(json);
                                NodeUtilities_PrepareJsonAndSendToRoot(59, 0, dataToSend);
                                free(dataToSend);
                                cJSON_Delete(json);
                            }
                        }
                    }
                }
                vPortFree(recv_cb->data);
                vPortFree(recv_cb->macStr);
                vPortFree(recv_cb);
                //ESP_LOGE(TAG, "MEMORY HEAP SIZE: %d", xPortGetFreeHeapSize());
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); //Leave this for watchdog
    }
}

#endif