#ifdef IPNODE
#include "includes/node_utilities.h"

//************************COMMAND MAPPING FOR NODE*******************
NodeCmnds_t NodeCommand[enumNodeCmndKey_TotalNumOfCommands] = 
{
    {254,                                           NULL},
    {enumNodeCmndKey_RestartNode,                   RestartNode},
    {enumNodeCmndKey_TestNode,                      TestNode},
    {enumNodeCmndKey_SetNodeOutput,                 SetNodeOutput},
    {enumNodeCmndKey_CreateGroup,                   CreateGroup},
    {enumNodeCmndKey_DeleteGroup,                   DeleteGroup},
    {enumNodeCmndKey_AssignSensorToSelf,            AssignSensorToSelf},
    {enumNodeCmndKey_EnableOrDisableSensorAction,   EnableOrDisableSensorAction},
    {enumNodeCmndKey_RemoveSensor,                  RemoveSensor},
    {enumNodeCmndKey_ChangePortPIR,                 ChangePortPIR},
    {enumNodeCmndKey_AssignSensorToNodeOrGroup,     AssignSensorToNodeOrGroup},
    {enumNodeCmndKey_OverrideNodeType,              OverrideNodeType},
    {enumNodeCmndKey_RemoveNodeTypeOverride,        RemoveNodeTypeOverride},
    {enumNodeCmndKey_GetDaisyChainedCurrent,        GetDaisyChainedCurrent},
    {enumNodeCmndKey_SendFirmwareVersion,           SendFirmwareVersion},
    {enumNodeCmndKey_GetNodeOutput,                 GetNodeOutput},
    {enumNodeCmndKey_GetParentRSSI,                 GetParentRSSI},
    {enumNodeCmndKey_GetNodeChildren,               GetNodeChildren},
    {enumNodeCmndKey_SetAutoSensorPairing,          SetAutoSensorPairing},
    {enumNodeCmndKey_SetSensorMeshID,               SetSensorMeshID},
    {enumNodeCmndKey_SendSensorFirmware,            SendSensorFirmware},
    {enumNodeCmndKey_ClearSensorCommand,            ClearSensorCommand},
    {enumNodeCmndKey_ClearNVS,                      ClearNVS}
};

#ifdef GATEWAY_IPNODE
NodeCmnds_t GWCommand[enumGatewayCmndKey_TotalNumOfCommands] = 
{
    {254,                                               NULL},
    {enumGatewayCmndKey_GW_Action,                      NULL},
    {enumGatewayCmndKey_GW_Query,                       NULL},
    {enumGatewayCmndKey_GW_GW_ROOT_find_OTA_partition,   NULL},
    {enumGatewayCmndKey_GW_GW_ROOT_OTA_end,             NULL},
    {enumGatewayCmndKey_GW_AT_CMD,                      NULL}
};
#endif

//************************COMMAND MAPPING FOR NODE*******************

static const char *TAG                  = "NodeUtilities";
static uint8_t ubyNodeCommandStartValue = 0;
uint8_t ubyNumOfCommands                = 0;
uint8_t ubyBaseNodeNumOfCommands        = 0;
uint8_t devType                         = 0;
uint8_t autoSensorPairing               = AUTO_SENSOR_PAIRING_DEFAULT;
size_t firmware_size                    = 0;
uint8_t sha_256[32]                     = {0};
char *dataPointsGlobal[30]              = {""};
uint8_t numberOfDataPoints              = 0;
bool rootReachable                      = false;
NodeCmnds_t *structptrCommand;
QueueHandle_t nodeReadQueue;
QueueHandle_t rootSendQueue;


bool NodeUtilities_ValidateAndExecuteCommand(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "Command Received: %d", structNodeReceived->ubyCommand);
    if (structNodeReceived->ubyCommand < (BASE_NODE_COMMAND_RANGE_STARTS + COMMAND_VALUE_SPAN))
    {
        for (uint8_t ubyIndex = 1; ubyIndex <= ubyBaseNodeNumOfCommands; ubyIndex++)
        {
            if (NodeCommand[ubyIndex].ubyKey == structNodeReceived->ubyCommand)
            {
                return NodeCommand[ubyIndex].node_fun(structNodeReceived);
            }
        }
    }
    else if ((structNodeReceived->ubyCommand >= ubyNodeCommandStartValue) && (structNodeReceived->ubyCommand < (ubyNodeCommandStartValue + COMMAND_VALUE_SPAN)))
    {
        for (uint8_t ubyIndex = 1; ubyIndex <= ubyNumOfCommands; ubyIndex++)
        {
            if (structptrCommand[ubyIndex].ubyKey == structNodeReceived->ubyCommand)
            {
                return structptrCommand[ubyIndex].node_fun(structNodeReceived);
            }
        }
    }
    ESP_LOGW(TAG, "Incorrect Command Received.");
    return false;
}

void NodeUtilities_CreateQueues()
{
    // created a queue with maxumum of 70 commands, and a item size of a char pointer
    nodeReadQueue = xQueueCreate(70, sizeof(char *));
    if (nodeReadQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create node read queue");
        // need to restart and let the backend know
    }

    rootSendQueue = xQueueCreate(100, sizeof(char *));
    if (rootSendQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create Root Send queue");
        // need to restart and let the backend know
    }
}

bool NodeUtilities_compareWithGroupMac(uint8_t *mac)
{
    uint8_t groupMac[3] = {0x01, 0x00, 0x5e};
    for (uint8_t i = 0; i < 3; i++)
    {
        if (groupMac[i] != mac[i])
            return false;
    }
    return true;
}

void NodeUtilities_InitGpios(uint64_t mask, gpio_mode_t mode)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = mode;
    io_conf.pin_bit_mask = mask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

// ADC channels mapped to their respective GPIOS
adc1_channel_t NodeUtilities_AdcChannelLookup(uint8_t gpio)
{
    switch (gpio)
    {
    case 36:
        return ADC1_CHANNEL_0;
    case 37:
        return ADC1_CHANNEL_1;
    case 38:
        return ADC1_CHANNEL_2;
    case 39:
        return ADC1_CHANNEL_3;
    case 32:
        return ADC1_CHANNEL_4;
    case 33:
        return ADC1_CHANNEL_5;
    case 34:
        return ADC1_CHANNEL_6;
    case 35:
        return ADC1_CHANNEL_7;
    default:
        return ADC1_CHANNEL_0;
    }
}

bool NodeUtilities_LoadAndDeleteGroup(nvs_handle nvsHandle)
{
    uint8_t groupMac[8] = {};
    size_t groupMacSize = sizeof(groupMac);
    esp_err_t err = nvs_get_blob(nvsHandle, "group", &groupMac, &groupMacSize);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return true;
    }
    else if (err != ESP_OK)
    {
        return false;
    }

    if (nvs_erase_key(nvsHandle, "group") != ESP_OK)
    {
        return false;
    }
    if (nvs_commit(nvsHandle) != ESP_OK)
    {
        return false;
    }

    if (esp_mesh_delete_group_id((mesh_addr_t *)groupMac, 1) != ESP_OK)
        return false;
    ESP_LOGI(TAG, "Prvious Group Deleted");
    return true;
}

void NodeUtilities_PrepareAndSendRegisterData()
{
    cJSON *registerData = cJSON_CreateObject();
    cJSON_AddItemToObject(registerData, "devID", cJSON_CreateString(deviceMACStr));
    cJSON_AddNumberToObject(registerData, "devType", devType);
    for (uint8_t i = 0; i < numberOfDataPoints; i++)
    {
        if (dataPointsGlobal[i] != NULL && strcmp(dataPointsGlobal[i], "") != 0)
            cJSON_AddNumberToObject(registerData, dataPointsGlobal[i], 0.000);
    }
    char *dataToSend = cJSON_PrintUnformatted(registerData);
    NodeUtilities_PrepareJsonAndSendToRoot(59, 0, dataToSend);
    free(dataToSend);
    cJSON_Delete(registerData);
}

void NodeUtilities_PrepareJsonAndSendToRoot(uint16_t ubyCommand, uint32_t uwValue, char *cptrString)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "cmnd", ubyCommand);
    cJSON_AddNumberToObject(response, "val", uwValue);
    cJSON_AddItemToObject(response, "str", cJSON_CreateString(cptrString));
    char *dataToSend = cJSON_PrintUnformatted(response);
    char *payload = (char *)pvPortMalloc((strlen(dataToSend) + 1) * sizeof(char));
    memset(payload, 0, strlen(dataToSend) + 1);
    strcpy(payload, dataToSend);
    
    // If SIM module is connected, then send the data to it
#ifdef GATEWAY_SIM7080
    SecondaryUtilities_PrepareJSONAndSendToAWS(ubyCommand, uwValue, cptrString);
#endif
    if (xQueueSendToBack(rootSendQueue, &payload, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(payload);
    }
    free(dataToSend);
    cJSON_Delete(response);
}

bool NodeUtilities_LoadGroup()
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;

    uint8_t groupMac[8] = {};
    size_t groupMacSize = sizeof(groupMac);
    esp_err_t err = nvs_get_blob(nvsHandle, "group", &groupMac, &groupMacSize);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(nvsHandle);
        return true;
    }
    else if (err != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
    if (esp_mesh_set_group_id((mesh_addr_t *)groupMac, 1) != ESP_OK)
        return false;
    return true;
}

bool NodeUtilities_GetAutoPairing()
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_get_u8(nvsHandle, "autoSensorPair", &autoSensorPairing) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
    return true;
}

void NodeUtilities_SendDataToGroup(uint8_t *ubyGroupMacAddr, char *cptrDataToSend)
{
    size_t size = strlen(cptrDataToSend);
    mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_MULTICAST, .compression = true, .group = true};
    mdf_err_t ret = mwifi_write(ubyGroupMacAddr, &data_type, cptrDataToSend, size, true);
    if (ret == MDF_OK)
    {
        ESP_LOGI(TAG, "Free heap on Node: %u", esp_get_free_heap_size());
    }
    else
    {
        // Report back to a error topic saying could not send
        ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
    }
}

void NodeUtilities_ParseArrVal(cJSON *cjVals, double *arrvals)
{
    cJSON *Iterator = NULL;
    uint8_t count = 0;
    cJSON_ArrayForEach(Iterator, cjVals)
    {
        if (cJSON_IsNumber(Iterator))
        {
            // ESP_LOGI(TAG, "Number %d, ", Iterator->valueint);
            arrvals[count] = Iterator->valuedouble;
            count++;
        }
        else
        {
            ESP_LOGI(TAG, "Invalid Number");
        }
    }
}

void NodeUtilities_SendToNode(uint8_t *macOfNode, char *dataToSend)
{
    mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_UNICAST, .compression = true};
    mdf_err_t ret = mwifi_write(macOfNode, &data_type, dataToSend, strlen(dataToSend) + 1, true);
    if (ret == MDF_OK)
    {
        ESP_LOGI(TAG, "Data Sent to Node");
    }
    else
    {
        // Report back to a error topic saying could not send
        ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
    }
}

void NodeUtilities_PrepareJsonAndSendDataToGroup(uint16_t ubyCommand, uint32_t uwValue, char *cptrString, uint8_t *ubyGroupMacAddr)
{
    cJSON *response = cJSON_CreateObject();
    cJSON *cjCmnd = cJSON_CreateNumber(ubyCommand);
    cJSON_AddItemToObject(response, "cmnd", cjCmnd);
    cJSON_AddNumberToObject(response, "val", uwValue);
    cJSON_AddItemToObject(response, "str", cJSON_CreateString(cptrString));
    ESP_LOGI(TAG, "Data Prepared to send to the Group");
    char *cptrDataToSend = cJSON_PrintUnformatted(response);
    ESP_LOGI(TAG, "Sending");
    NodeUtilities_SendDataToGroup(ubyGroupMacAddr, cptrDataToSend);
    free(cptrDataToSend);
    cJSON_Delete(response);
}

// Reads the voltage from the ADC, does noise smoothing using samples and moving average
uint32_t NodeUtilities_ReadMilliVolts(uint8_t gpio, uint16_t samples, esp_adc_cal_characteristics_t *adc_chars)
{
    adc1_channel_t adcChannel = NodeUtilities_AdcChannelLookup(gpio);
    uint32_t adcRaw = 0;
    for (int i = 0; i < samples; i++)
    {
        adcRaw += adc1_get_raw(adcChannel);
    }
    adcRaw /= samples;
    return esp_adc_cal_raw_to_voltage(adcRaw, adc_chars); // return milli volts
}

void NodeUtilities_InitialiseAndSetNodeOutput()
{
    NodeUtilities_InitGpios(NODE_OUTPUT_PIN_MASk, GPIO_MODE_OUTPUT);
    gpio_set_level(NODE_OUTPUT_PIN_DEFAULT, 0);
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    uint8_t nodeOutput;
    if (nvs_get_u8(nvsHandle, "nodeOutput", &nodeOutput) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    nvs_close(nvsHandle);
    gpio_set_level(NODE_OUTPUT_PIN_DEFAULT, nodeOutput);
}

void NodeUtilities_overrideNodeType(uint8_t nodeType)
{
    ESP_LOGW(TAG, "Override NodeType");
    switch (nodeType)
    {
    //GW required changes
    case 4:
#ifdef GATEWAY_IPNODE
        NodeUtilities_initiateGatewayNode();
#else
        NodeUtilities_initiateNothingNode();
#endif
        break;

    default:
        break;
    }
}

#ifdef GATEWAY_IPNODE
void NodeUtilities_initiateGatewayNode()
{
    ESP_LOGW(TAG, "I am SpacrGateway");
    ubyNodeCommandStartValue = GATEWAY_NODE_COMMAND_RANGE_STARTS;
    structptrCommand = GWCommand;
    ubyNumOfCommands = sizeof(GWCommand) / sizeof(NodeCmnds_t);
    devType = 4;
    Initialize_Gateway();
}
#endif

void NodeUtilities_initiateNothingNode()
{
    ESP_LOGW(TAG, "I am SpacrNothing");
    devType = 4;
}


void NodeUtilities_WhoAmI()
{
    ubyBaseNodeNumOfCommands = sizeof(NodeCommand) / sizeof(NodeCmnds_t);
    uint8_t nodeType = 0;
    //beginWiper();
    // checking if node ovveride exists
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) == ESP_OK)
    {
        nvs_get_u8(nvsHandle, "nodeType", &nodeType);
        nvs_close(nvsHandle);
    }
    //temp change for the Node to act as nothing
    nodeType = 4;
    if (nodeType != 0)
    {
        NodeUtilities_overrideNodeType(nodeType);
        return;
    }
    /*
    // Override does not exist, so detect device autmatically
    int iWiper = getWiper();
    // init digiRead gpios
    uint64_t inputMaskTest = ((1ULL << adc4));
    uint64_t outputMaskWhoAmI = ((1ULL << D4) | (1ULL << D3));
    NodeUtilities_InitGpios(inputMaskTest, GPIO_MODE_INPUT);
    NodeUtilities_InitGpios(outputMaskWhoAmI, GPIO_MODE_OUTPUT);
    gpio_set_level(D4, 0);
    gpio_set_level(D3, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    uint8_t level = gpio_get_level(adc4);
    // FOR SPACRIO WE HAVE TO MANUALLY ADD A WIRE FROM 5V TO THE DIGIREAD
    if (level == 1 && iWiper == 256)
    {
        //NodeUtilities_initiateSpacrIO();
    }
    else
    {
        iWiper = getWiper();
        if (iWiper > 256 || iWiper < 0)
        {
            //NodeUtilities_initiateContactor();
        }
        else
        {
           // NodeUtilities_initiatePowerNode();
        }
    }
    */
}

void NodeUtilities_SendSensorStatusUpdate(int status, espnow_addr_t sensorAddress)
{
    ESP_LOGI(TAG, "Sending sensor status update");
    cJSON *sensorData = cJSON_CreateObject();
    char stringMac[18];
    Utilities_MacToString((uint8_t *)sensorAddress, stringMac, 18);
    cJSON_AddItemToObject(sensorData, "devID", cJSON_CreateString(stringMac));
    cJSON_AddNumberToObject(sensorData, "status", status);
    char *dataToSend = cJSON_PrintUnformatted(sensorData);
    NodeUtilities_PrepareJsonAndSendToRoot(60, 0, dataToSend);
    cJSON_Delete(sensorData);
    free(dataToSend);
}

esp_err_t ota_initiator_data_cb(size_t src_offset, void *dst, size_t size)
{
    static const esp_partition_t *data_partition = NULL;

    if (!data_partition)
    {
        data_partition = esp_ota_get_next_update_partition(NULL);
    }

    return esp_partition_read(data_partition, src_offset, dst, size);
}

void NodeUtilities_SendFirmware(size_t firmware_size, uint8_t sha[ESPNOW_OTA_HASH_LEN], uint8_t dest_addr[6])
{
    esp_err_t ret = ESP_OK;
    uint32_t start_time = xTaskGetTickCount();
    espnow_ota_result_t espnow_ota_result = {0};

    ESP_LOGW(TAG, "espnow wait ota num: %d", 1);
    uint8_t addrs[1][6];
    memcpy(addrs, dest_addr, 6);
    ret = espnow_ota_initator_send(addrs, 1, sha, firmware_size, ota_initiator_data_cb, &espnow_ota_result);
    ESP_ERROR_GOTO(ret != ESP_OK, EXIT, "<%s> espnow_ota_initator_send", esp_err_to_name(ret));

    if (espnow_ota_result.successed_num == 0)
    {
        ESP_LOGW(TAG, "Devices upgrade failed, unfinished_num: %d", espnow_ota_result.unfinished_num);
        goto EXIT;
    }

    ESP_LOGI(TAG, "Firmware is sent to the device to complete, Spend time: %ds",
             (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);
    ESP_LOGI(TAG, "Devices upgrade completed, successed_num: %d, unfinished_num: %d",
             espnow_ota_result.successed_num, espnow_ota_result.unfinished_num);

EXIT:
    // 0 = dead, 1 = alive, 2 = restarting, 3 = Firmware updates, 4 = Firmware update failed
    for (int i = 0; i < espnow_ota_result.successed_num; i++)
    {
        NodeUtilities_SendSensorStatusUpdate(DEVICE_STATUS_UPDATE_SUCCESS, espnow_ota_result.successed_addr[i]);
    }
    for (int i = 0; i < espnow_ota_result.unfinished_num; i++)
    {
        NodeUtilities_SendSensorStatusUpdate(DEVICE_STATUS_UPDATE_FAIL, espnow_ota_result.unfinished_addr[i]);
    }

    espnow_ota_initator_result_free(&espnow_ota_result);
}

#endif