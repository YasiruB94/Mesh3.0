#ifdef IPNODE
#include "includes/node_utilities.h"

//************************COMMAND MAPPING FOR NODE*******************
NodeCmnds_t NodeCommand[enumNodeCmndKey_TotalNumOfCommands] = {
    {254, NULL},
    {enumNodeCmndKey_RestartNode, RestartNode},
    {enumNodeCmndKey_TestNode, TestNode},
    {enumNodeCmndKey_SetNodeOutput, SetNodeOutput},
    {enumNodeCmndKey_CreateGroup, CreateGroup},
    {enumNodeCmndKey_DeleteGroup, DeleteGroup},
    {enumNodeCmndKey_AssignSensorToSelf, AssignSensorToSelf},
    {enumNodeCmndKey_EnableOrDisableSensorAction, EnableOrDisableSensorAction},
    {enumNodeCmndKey_RemoveSensor, RemoveSensor},
    {enumNodeCmndKey_ChangePortPIR, ChangePortPIR},
    {enumNodeCmndKey_AssignSensorToNodeOrGroup, AssignSensorToNodeOrGroup},
    {enumNodeCmndKey_OverrideNodeType, OverrideNodeType},
    {enumNodeCmndKey_RemoveNodeTypeOverride, RemoveNodeTypeOverride},
    {enumNodeCmndKey_GetDaisyChainedCurrent, GetDaisyChainedCurrent},
    {enumNodeCmndKey_GetFirmwareVersion, GetFirmwareVersion},
    {enumNodeCmndKey_GetNodeOutput, GetNodeOutput},
    {enumNodeCmndKey_GetParentRSSI, GetParentRSSI},
    {enumNodeCmndKey_GetNodeChildren, GetNodeChildren}};

//GW required changes: Match commands for the GW
NodeCmnds_t GWCommand[enumGatewayCmndKey_TotalNumOfCommands] = {
    {254, NULL},
    {enumGatewayCmndKey_GW_Action, GW_Process_Action_Command},
    {enumGatewayCmndKey_GW_Query, GW_Process_Query_Command},
    {enumGatewayCmndKey_GW_OTA_Begin, GW_Process_OTA_Command_Begin},
    {enumGatewayCmndKey_GW_OTA_End, GW_Process_OTA_Command_End},
    {enumGatewayCmndKey_GW_AT_CMD, GW_Process_AT_Command}
    };

NodeCmnds_t SPCommand[enumPowerCmndKey_TotalNumOfCommands] = {
    {254, NULL},
    {enumPowerCmndKey_SP_SetVoltage, SP_SetVoltage},
    {enumPowerCmndKey_SP_SetPWMPort0, SP_SetPWMPort0},
    {enumPowerCmndKey_SP_SetPWMPort1, SP_SetPWMPort1},
    {enumPowerCmndKey_SP_SetPWMBothPort, SP_SetPWMBothPort},
    {enumPowerCmndKey_SP_EnableCurrentSubmission, SP_EnableCurrentSubmission}};

NodeCmnds_t SIOCommand[enumIOCmndKey_TotalNumOfCommands] = {
    {254, NULL},
    {enumIOCmndKey_SIO_SetVoltage, SIO_SetVoltage},
    {enumIOCmndKey_SIO_SelectADCs, SIO_SelectADCs},
    {enumIOCmndKey_SIO_SetADCPeriod, SIO_SetADCPeriod},
    {enumIOCmndKey_SIO_EnableDisableSub, SIO_EnableDisableSub},
    {enumIOCmndKey_SIO_SetRelay, SIO_SetRelay}};

NodeCmnds_t SCCommand[enumContactorCmndKey_TotalNumOfCommands] = {
    {254, NULL},
    {enumContactorCmndKey_SC_SetRelays, SC_SetRelays},
    {enumContactorCmndKey_SC_SelectADCs, SC_SelectADCs},
    {enumContactorCmndKey_SC_SetADCPeriod, SC_SetADCPeriod}};
//************************COMMAND MAPPING FOR NODE*******************

static const char *TAG = "NodeUtilities";

static uint8_t ubyNodeCommandStartValue = 0;
NodeCmnds_t *structptrCommand;
uint8_t ubyNumOfCommands = 0;
uint8_t ubyBaseNodeNumOfCommands = 0;

uint8_t nodeOutputPin = 23;
uint8_t devType = 0;

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
    //created a queue with maxumum of 70 commands, and a item size of a char pointer
    nodeReadQueue = xQueueCreate(70, sizeof(char *));
    if (nodeReadQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create node read queue");
        //need to restart and let the backend know
    }

    rootSendQueue = xQueueCreate(20, sizeof(char *));
    if (rootSendQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create Root Send queue");
        //need to restart and let the backend know
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

//ADC channels mapped to their respective GPIOS
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

bool NodeUtilities_LoadAllNodeGroups()
{
    char groupMacAdd[16] = "01:00:5e:ae:ae:";
    //init nvs handle
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;

    //getting the size of the mac list from the blob
    size_t groupListSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, "groups", NULL, &groupListSize);
    if (err != ESP_OK || groupListSize == 0)
    {
        nvs_close(nvsHandle);
        return false;
    }
    //getting the group mac list
    uint8_t groupIDS[groupListSize];
    if (nvs_get_blob(nvsHandle, "groups", &groupIDS, &groupListSize) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
    //Loop over the IDS, convert them to string, append them to the groupID mac string and then convert them back to mac and set as group ID
    for (uint8_t i = 0; i < groupListSize; i++)
    {
        char groupIDStr[3];
        char strMac[18];
        uint8_t numMac[6];
        if (itoa(groupIDS[i], groupIDStr, 10) == NULL)
            return false;
        strcpy(strMac, groupMacAdd);
        strcat(strMac, groupIDStr);
        ESP_LOGE(TAG, "strMac %d: %s", i, strMac);
        if (!Utilities_StringToMac(strMac, numMac))
            return false;
        if (esp_mesh_set_group_id((mesh_addr_t *)numMac, 1) != ESP_OK)
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
        //Report back to a error topic saying could not send
        ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
    }
}

void NodeUtilities_ParseArrVal(cJSON *cjVals, uint8_t *arrvals)
{
    cJSON *Iterator = NULL;
    uint8_t count = 0;
    cJSON_ArrayForEach(Iterator, cjVals)
    {
        if (cJSON_IsNumber(Iterator))
        {
            //ESP_LOGI(TAG, "Number %d, ", Iterator->valueint);
            arrvals[count] = (uint8_t)Iterator->valueint;
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
        //Report back to a error topic saying could not send
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

//Reads the voltage from the ADC, does noise smoothing using samples and moving average
uint32_t NodeUtilities_ReadMilliVolts(uint8_t gpio, uint8_t samples, esp_adc_cal_characteristics_t *adc_chars)
{
    adc1_channel_t adcChannel = NodeUtilities_AdcChannelLookup(gpio);
    uint32_t adcRaw = 0;
    for (int i = 0; i < samples; i++)
    {
        adcRaw += adc1_get_raw(adcChannel);
    }
    adcRaw /= samples;
    return esp_adc_cal_raw_to_voltage(adcRaw, adc_chars); //return milli volts
}

void NodeUtilities_InitialiseAndSetNodeOutput()
{
    uint64_t outputNodeMask = ((1ULL << nodeOutputPin));
    NodeUtilities_InitGpios(outputNodeMask, GPIO_MODE_OUTPUT);
    gpio_set_level(nodeOutputPin, 0);
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
    gpio_set_level(nodeOutputPin, nodeOutput);
}

void NodeUtilities_overrideNodeType(uint8_t nodeType)
{
    ESP_LOGW(TAG, "Override NodeType");
    switch (nodeType)
    {
    case 1:
        NodeUtilities_initiatePowerNode();
        break;

    case 2:
        NodeUtilities_initiateSpacrIO();
        break;

    case 3:
        NodeUtilities_initiateContactor();
        break;
    //GW required changes
    case 4:
        NodeUtilities_initiateGatewayNode();
        break;
    }
}

//GW required changes: init the GW required functions
void NodeUtilities_initiateGatewayNode()
{
    ESP_LOGW(TAG, "I am SpacrGateway");
    ubyNodeCommandStartValue = GATEWAY_NODE_COMMAND_RANGE_STARTS;
    structptrCommand = GWCommand;
    ubyNumOfCommands = sizeof(GWCommand) / sizeof(NodeCmnds_t);
    devType = 4;
    Initialize_Gateway();
}

void NodeUtilities_initiatePowerNode()
{
    ESP_LOGW(TAG, "I am SpacrPower");
    ubyNodeCommandStartValue = POWER_NODE_COMMAND_RANGE_STARTS;
    structptrCommand = SPCommand;
    ubyNumOfCommands = sizeof(SPCommand) / sizeof(NodeCmnds_t);
    devType = 1;
    //initialising values here for and loading previously stored values
    SP_Init();
}

void NodeUtilities_initiateSpacrIO()
{
    ESP_LOGW(TAG, "I am SpacrIO");
    ubyNodeCommandStartValue = IO_NODE_COMMAND_RANGE_STARTS;
    structptrCommand = SIOCommand;
    ubyNumOfCommands = sizeof(SIOCommand) / sizeof(NodeCmnds_t);
    devType = 2;
    SIO_Init();
}

void NodeUtilities_initiateContactor()
{
    ESP_LOGW(TAG, "I am SpacrContactor");
    ubyNodeCommandStartValue = CONTACTOR_NODE_COMMAND_RANGE_STARTS;
    structptrCommand = SCCommand;
    ubyNumOfCommands = sizeof(SCCommand) / sizeof(NodeCmnds_t);
    devType = 3;
    //Initialising things here for spacr contactor
    SC_Init();
}

void NodeUtilities_WhoAmI()
{
    ubyBaseNodeNumOfCommands = sizeof(NodeCommand) / sizeof(NodeCmnds_t);
    uint8_t nodeType = 0;
    beginWiper();
    //checking if node ovveride exists
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) == ESP_OK)
    {
        nvs_get_u8(nvsHandle, "nodeType", &nodeType);
        nvs_close(nvsHandle);
    }
    //GW required changes: temp change for the Node to act as a GW
    nodeType = 4;
    if (nodeType != 0)
    {
        NodeUtilities_overrideNodeType(nodeType);
        return;
    }

    //Override does not exist, so detect device autmatically
    int iWiper = getWiper();
    //init digiRead gpios
    uint64_t inputMaskTest = ((1ULL << adc4));
    uint64_t outputMaskWhoAmI = ((1ULL << D4) | (1ULL << D3));
    NodeUtilities_InitGpios(inputMaskTest, GPIO_MODE_INPUT);
    NodeUtilities_InitGpios(outputMaskWhoAmI, GPIO_MODE_OUTPUT);
    gpio_set_level(D4, 0);
    gpio_set_level(D3, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    uint8_t level = gpio_get_level(adc4);
    //FOR SPACRIO WE HAVE TO MANUALLY ADD A WIRE FROM 5V TO THE DIGIREAD
    if (level == 1 && iWiper == 256)
    {
        NodeUtilities_initiateSpacrIO();
    }
    else
    {
        iWiper = getWiper();
        if (iWiper > 256 || iWiper < 0)
        {
            NodeUtilities_initiateContactor();
        }
        else
        {
            NodeUtilities_initiatePowerNode();
        }
    }
}

#endif