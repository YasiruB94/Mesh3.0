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
                // GW required changes: OTA data does not come in the form of JSON. therefore the data is parsed to GW OTA function.
                // If the function does not expect OTA data, it returns immediately.
                // This is commented out now, because the NODE does not behave as a GATEWAY 
#ifdef GATEWAY_IPNODE
                bool OTA_data = GW_Process_OTA_Command_Data(cptrNodeData);
#else
                bool OTA_data = false;
#endif
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
                    // If the object cannot be parsed, but it indeed is a json object, it will not append the devID and devType here.
                    if (!OTA_data)
                    {
                        //NodeUtilities_PrepareJsonAndSendToRoot(65, 0, cptrNodeData);
                    }
                }
                else
                {
                    // Parse data
                    if (!cJSON_HasObjectItem(json, "devID"))
                    {
                        cJSON_AddItemToObject(json, "devID", cJSON_CreateString(deviceMACStr));
                        cJSON_AddNumberToObject(json, "devType", devType);
                    }
                    char *dataToSend = cJSON_PrintUnformatted(json);
                    cJSON *cjCommand = cJSON_GetObjectItem(json, "cmnd");
                    cJSON *cjValue = cJSON_GetObjectItem(json, "val");
                    cJSON *cjString = cJSON_GetObjectItem(json, "str");
                    if (cJSON_IsNumber(cjCommand) &&
                        (cJSON_IsNumber(cjValue) || cJSON_IsArray(cjValue)) &&
                        cJSON_IsString(cjString))
                    {
                        structNodeReceived->ubyCommand = cjCommand->valueint;
                        structNodeReceived->dValue = cjValue->valuedouble;
                        structNodeReceived->cptrString = cjString->valuestring;
                        structNodeReceived->arrValueSize = 0;
                        if (cJSON_IsArray(cjValue))
                        {
                            isArray = true;
                            structNodeReceived->arrValueSize = cJSON_GetArraySize(cjValue);
                            structNodeReceived->arrValues = (double *)malloc(structNodeReceived->arrValueSize * sizeof(double));
                            NodeUtilities_ParseArrVal(cjValue, structNodeReceived->arrValues);
                        }
                        bool returnVal = NodeUtilities_ValidateAndExecuteCommand(structNodeReceived);
                        if (returnVal)
                            NodeUtilities_PrepareJsonAndSendToRoot(64, 0, dataToSend);
                        else
                            NodeUtilities_PrepareJsonAndSendToRoot(65, 0, dataToSend);
                        free(dataToSend);
                        goto NodeOperations_CommandExecutionTask_End;
                    }

                    // GW required changes: sometimes OTA data is mis-represented as JSON formatted, even though it is not.
                    // therefore, first check if the current data is OTA related, and if so, then dont send information to AWS
                    if (!OTA_data)
                    {
                        NodeUtilities_PrepareJsonAndSendToRoot(65, 0, dataToSend);
                    }
                    free(dataToSend);
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
        vTaskDelay(10 / portTICK_RATE_MS); // Leave this for watchdog
    }
}

void NodeOperations_RootSendTask(void *arg)
{
    char *rootSendData = NULL;
    while (true)
    {
        if (rootSendQueue != NULL && mwifi_get_root_status() && rootReachable) // && !g_rootless_flag)
        {
            if (xQueueReceive(rootSendQueue, &rootSendData, (TickType_t)0))
            {
                mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_UNICAST, .compression = true};
                mdf_err_t ret = mwifi_write(NULL, &data_type, rootSendData, strlen(rootSendData) + 1, true);
                // TODO@sagar448 #16 handling error if the mwifi write function did not successfully send!
                if (ret == MDF_OK)
                {
                    ESP_LOGI(TAG, "Data Sent to Root");
                }
                else
                {
                    // Report back to a error topic saying could not send
                    ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
                }
                vPortFree(rootSendData);
            }
        }
        // else if (g_rootless_flag)
        // {
        //     // ESP_LOGI(TAG, "node is rootless");
        // }
        vTaskDelay(10 / portTICK_RATE_MS); // Leave this for watchdog
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
            // ESP_LOGI(TAG, "Here ");
            continue;
        }
        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "mwifi_read, ret: %s", mdf_err_to_name(ret));
        if (data_type.upgrade)
        {
            {
                // MDF_LOGW("node receiving upgraded firmware");
                ret = mupgrade_handle(src_addr, data, size);
                MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            }
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
                // Report back to Root here letting us know that the queue is full and to wait for the next command
                // The queue most likely will neveer fill up but you never know
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
    EspNowStruct *sensorDataRecv;
    while (true)
    {
        if (espNowQueue != NULL)
        {
            if (xQueueReceive(espNowQueue, &sensorDataRecv, (TickType_t)0))
            {
                // ESP_LOGW(TAG, "Sensor Secret recieved by Sensor %s", sensorDataRecv->sensorSecret);
                Sensor_SendPendingMessage(sensorDataRecv);
                if (sensorDataRecv->mode == 0 && (strcmp(sensorDataRecv->meshID, " ") == 0 || strcmp(sensorDataRecv->meshID, MESH_ID) == 0))
                {
                    if (autoSensorPairing)
                        Sensor_SendFriendInfo(sensorDataRecv->mac_addr);
                    else if (Sensor_AlreadyExists(sensorDataRecv))
                    {
                        Sensor_SendFriendInfo(sensorDataRecv->mac_addr);
                    }
                }
                else if (sensorDataRecv->mode == 1)
                {
                    // We should check if a node or a group needs this sensor data and send right away!
                    if (!EspNow_CheckAction(sensorDataRecv))
                        goto EXIT;
                    // Convert the data to cJson to submit to the root;
                    char *sensorDataToSend = Sensor_CastStructToCjson(sensorDataRecv);
                    // Checking to see if the sensor device needs to be registered
                    if (sensorDataRecv->registerDev == 1)
                        NodeUtilities_PrepareJsonAndSendToRoot(59, 0, sensorDataToSend);
                    // We do not want it to send to both topics as if the device does not exist then sensor data lambda will not execute anyways
                    else
                        NodeUtilities_PrepareJsonAndSendToRoot(58, 0, sensorDataToSend);
                    free(sensorDataToSend);
                }

            EXIT:
                vPortFree(sensorDataRecv->data);
                vPortFree(sensorDataRecv);
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); // Leave this for watchdog
    }
}

void NodeOperations_ReadFromSensorTask(void *arg)
{
    esp_err_t ret = ESP_OK;
    uint32_t count = 0;
    char *data = ESP_MALLOC(ESPNOW_DATA_LEN);
    size_t size = ESPNOW_DATA_LEN;
    uint8_t addr[ESPNOW_ADDR_LEN] = {0};
    wifi_pkt_rx_ctrl_t rx_ctrl = {0};

    ESP_LOGI(TAG, "Read from sensor task is running");

    for (;;)
    {
        ret = espnow_recv(ESPNOW_TYPE_DATA, addr, data, &size, &rx_ctrl, portMAX_DELAY);
        vTaskDelay(10 / portTICK_RATE_MS); // Leave this for watchdog
        ESP_ERROR_CONTINUE(ret != ESP_OK, "<%s>", esp_err_to_name(ret));

        ESP_LOGI(TAG, "espnow_recv, <%d> [" MACSTR "][%d][%d][%d]: %.*s",
                 count++, MAC2STR(addr), rx_ctrl.channel, rx_ctrl.rssi, size, size, data);
        EspNow_RecvCb(addr, (uint8_t *)data, (int)size);
    }

    ESP_LOGW(TAG, "Read from sensor task is exit");

    ESP_FREE(data);
    vTaskDelete(NULL);
}
#endif