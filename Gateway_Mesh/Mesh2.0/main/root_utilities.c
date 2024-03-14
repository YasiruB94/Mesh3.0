#ifdef ROOT
#include "Includes/root_utilities.h"
#include "gw_includes/ota_agent.h"
#include "includes/SpacrGateway_commands.h"
#include "errno.h"

//*********************ROOT COMMANDS********************************
RootCmnds_t RootCommand[enumRootCmndKey_TotalNumOfCommands] = 
{
    {enumRootCmndKey_RestartRoot                , RestartRoot},
    {enumRootCmndKey_TestRoot                   , TestRoot},
    {enumRootCmndKey_UpdateRootFW               , UpdateRootFW},
    {enumRootCmndKey_TestGroup                  , TestGroup},
    {enumRootCmndKey_SendGroupCmd               , SendGroupCmd},
    {enumRootCmndKey_AddOrgInfo                 , AddOrgInfo},
    {enumRootCmndKey_PublishSensorData          , PublishSensorData},
    {enumRootCmndKey_PublishRegisterDevice      , PublishRegisterDevice},
    {enumRootCmndKey_MessageToAWS               , MessageToAWS},
    {enumRootCmndKey_UpdateNodeFW               , UpdateFW},
    {enumRootCmndKey_CreateGroup                , CreateGroup},
    {enumRootCmndKey_DeleteGroup                , DeleteGroup},
    {enumRootCmndKey_PublishNodeControlSuccess  , PublishNodeControlSuccess},
    {enumRootCmndKey_PublishNodeControlFail     , PublishNodeControlFail},
    {enumRootCmndKey_UpdateSensorFW             , UpdateFW}, // note UpdateNodeFW and UpdateSensorFW point to the same command. They have minor variations in handling, so the code is shared
    {enumRootCmndKey_UpdateCenseFW              , UpdateCenceFW}
};

// All commands that are called from the node will not have the usrID key, so the root will not require that.
#define NUM_ROOT_COMMANDS_FROM_NODE 5
const int ROOT_COMMANDS_FROM_NODE[NUM_ROOT_COMMANDS_FROM_NODE] = {58, 59, 60, 64, 65};
//*********************ROOT COMMANDS********************************

//*********ROOT Global variables****************
char orgID[25] = "";
char *sensorLogTopic = NULL;
char *registerLogTopic = NULL;
char *logTopic = NULL;
char *controlLogTopicSuccess = NULL;
char *controlLogTopicFail = NULL;
//*********ROOT Global variables****************

static const char *TAG = "RootUtility";

QueueHandle_t nodeCommandQueue;
QueueHandle_t rootCommandQueue;
QueueHandle_t AWSPublishQueue;

bool RootUtilities_ParseNodeAddressAndData(MeshStruct_t *structRootWrite, char *cptrRcvdData)
{
    // getting the node address, the first strtok is ignored
    strtok(cptrRcvdData, "/");
    char *cptrData = NULL;
    // second strtok is the node mac in string
    char *nodeMacStr = strtok(NULL, "{");
    structRootWrite->ubyNumOfNodes = 1;
    // Verify the Node Mac Address
    if (nodeMacStr)
    {
        if (Utilities_StringToMac(nodeMacStr, structRootWrite->ubyNodeMac) == true)
        {
            // regardless if node exists or not we want to parse the data
            bool nodeVerified = RootUtilities_NodeAddressVerification(structRootWrite);
            // Last part of string is the data to send
            cptrData = strtok(NULL, "");
            if (cptrData)
            {
                // Adding '{' at the beginning of the Json
                memmove(structRootWrite->cReceivedData + 1, cptrData, strlen(cptrData));
                *structRootWrite->cReceivedData = '{';
                cptrData = NULL;
                return nodeVerified;
            }
        }
    }
    // Report back to a error topic saying in valid node address received
    ESP_LOGE(TAG, "RootUtilities_ParseNodeAddressAndData could not parse data");
    return false;
}

void RootUtilities_SendDataToAWS(char *cptrTopic, char *cptrPayload)
{
    ESP_LOGI(TAG, "RootUtilities_SendDataToAWS receieved %s", cptrPayload);
    AWSPub_t *stuctAWSPub = pvPortMalloc(sizeof(AWSPub_t));
    stuctAWSPub->cptrPayload = (char *)pvPortMalloc((strlen(cptrPayload) + 1) * sizeof(char));
    stuctAWSPub->cptrTopic = (char *)pvPortMalloc((strlen(cptrTopic) + 1) * sizeof(char));
    memset(stuctAWSPub->cptrPayload, 0, strlen(cptrPayload));
    memset(stuctAWSPub->cptrTopic, 0, strlen(cptrTopic));
    strcpy(stuctAWSPub->cptrPayload, cptrPayload);
    strcpy(stuctAWSPub->cptrTopic, cptrTopic);

// if it is a GATEWAY_SIM7080, divert the traffic to the queue the SIM7080 handles
#ifdef GATEWAY_SIM7080
    if (xQueueSendToBack(SIM7080_AWS_Tx_queue, &stuctAWSPub, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(stuctAWSPub->cptrPayload);
        vPortFree(stuctAWSPub->cptrTopic);
        vPortFree(stuctAWSPub);
        // Report back to AWS here letting us know that the queue is full
    }
#else
    if (xQueueSendToBack(AWSPublishQueue, &stuctAWSPub, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(stuctAWSPub->cptrPayload);
        vPortFree(stuctAWSPub->cptrTopic);
        vPortFree(stuctAWSPub);
        // Report back to AWS here letting us know that the queue is full
    }
#endif
}

// 0 = dead, 1 = alive, 2 = restarting, 3 = Firmware updates, 4 = Firmware update failed
bool RootUtilities_SendStatusUpdate(char *devID, uint8_t status)
{
    cJSON *rootData = cJSON_CreateObject();
    if (rootData == NULL)
        return false;
    cJSON *devMacStr = cJSON_CreateString(devID);
    if (devMacStr == NULL)
        return false;
    cJSON_AddItemToObject(rootData, "devID", devMacStr);
    cJSON_AddNumberToObject(rootData, "status", status);
    char *dataToSend = cJSON_PrintUnformatted(rootData);
    RootUtilities_SendDataToAWS(logTopic, dataToSend);
    cJSON_Delete(rootData);
    free(dataToSend);
    return true;
}

void RootUtilities_CreateQueues()
{
    // created a queue with maxumum of 10 commands, and a item size of a char pointer
    nodeCommandQueue = xQueueCreate(10, sizeof(char *));
    if (nodeCommandQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create node queue");
        // need to restart and let the backend know
    }

    rootCommandQueue = xQueueCreate(25, sizeof(char *));
    if (rootCommandQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create root queue");
        // need to restart and let the backend know
    }

    AWSPublishQueue = xQueueCreate(25, sizeof(AWSPub_t *));
    if (AWSPublishQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create root queue");
        // need to restart and let the backend know
    }
}

void RootUtilities_loadOrgInfo()
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    size_t sizeOfOrgID;
    if (nvs_get_str(nvsHandle, "orgID", NULL, &sizeOfOrgID) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    char orgIDLocal[sizeOfOrgID];
    if (nvs_get_str(nvsHandle, "orgID", orgIDLocal, &sizeOfOrgID) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    nvs_close(nvsHandle);
    strcpy(orgID, orgIDLocal);
    ESP_LOGI(TAG, "This is the org ID %s", orgID);
}

uint8_t RootUtilities_ValidateAndExecuteCommand(uint8_t ubyCommand, uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs, char *usrID)
{
    // ESP_LOGI(TAG, "Command Received: %d", ubyCommand);
    // before checking if the command is intended for the ROOT, check if the command validity for a GATEWAY
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
    if(ubyCommand > (ROOT_NODE_COMMAND_RANGE_STARTS + COMMAND_VALUE_SPAN))
    {
        NodeStruct_t *structNodeReceived = malloc(sizeof(NodeStruct_t));
        structNodeReceived->cptrString = cptrString;
        structNodeReceived->dValue = uwValue;
        structNodeReceived->ubyCommand = ubyCommand;
        bool result = SecondaryUtilities_ValidateAndExecuteCommand(structNodeReceived);
        free(structNodeReceived);
        if(result)
        {
            return 1;
        }
    }
#endif
    if ((ubyCommand >= ROOT_NODE_COMMAND_RANGE_STARTS) && (ubyCommand < (ROOT_NODE_COMMAND_RANGE_STARTS + COMMAND_VALUE_SPAN)))
    {
        for (uint8_t ubyIndex = 0; ubyIndex < enumRootCmndKey_TotalNumOfCommands; ubyIndex++)
        {
            if (RootCommand[ubyIndex].ubyKey == ubyCommand)
            {
                RootStruct_t *structRootReceived = malloc(sizeof(RootStruct_t));
                structRootReceived->usrID = usrID;
                structRootReceived->ubyCommand = ubyCommand;
                structRootReceived->uwValue = uwValue;
                structRootReceived->cptrString = cptrString;
                structRootReceived->ubyptrMacs = ubyptrMacs;
                uint8_t ret = RootCommand[ubyIndex].root_fun(structRootReceived);
                free(structRootReceived);
                // ESP_LOGI(TAG, "ret value is %d", ret);
                return ret;
            }
        }
    }
    // Send incorrect Command Received Error
    ESP_LOGW(TAG, "Incorrect Command Received.");
    return 0;
}

bool RootUtilities_SendDataToGroup(MeshStruct_t *structRootWrite)
{
    size_t size = strlen(structRootWrite->cReceivedData) + 1;
    mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_MULTICAST, .compression = true, .group = true};
    ESP_LOGI(TAG, "Data to Send: %s, len: %d", structRootWrite->cReceivedData, strlen(structRootWrite->cReceivedData));

    ESP_LOGI(TAG, "Group Addr: %x:%x:%x:%x:%x:%x", structRootWrite->ubyNodeMac[0], structRootWrite->ubyNodeMac[1], structRootWrite->ubyNodeMac[2], structRootWrite->ubyNodeMac[3],
             structRootWrite->ubyNodeMac[4], structRootWrite->ubyNodeMac[5]);
    // const uint8_t group_id_list[6] = {0x01, 0x00, 0x5e, 0xae, 0xae, 0xae};
    mdf_err_t ret = mwifi_write(structRootWrite->ubyNodeMac, &data_type, (char *)structRootWrite->cReceivedData, size, true);
    // mdf_err_t ret = mwifi_write(group_id_list, &data_type, (char *)structRootWrite->cReceivedData, size, true);
    if (ret == MDF_OK)
    {
        ESP_LOGI(TAG, "Free heap on Root: %u", esp_get_free_heap_size());
        return true;
    }
    else
    {
        // Report back to a error topic saying could not send
        ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
        return false;
    }
}

bool RootUtilities_NodeAddressVerification(MeshStruct_t *structRootWrite)
{
    ESP_LOGI(TAG, "Free Heap Before: %d", xPortGetFreeHeapSize());
    ssize_t routing_table_size = esp_mesh_get_routing_table_size();
    // ESP_LOGI(TAG, "Routing Table Size is: %d", routing_table_size);
    mesh_addr_t *routing_table = malloc(routing_table_size * sizeof(mesh_addr_t) + 1);
    mdf_err_t ret = esp_mesh_get_routing_table(routing_table, routing_table_size * sizeof(mesh_addr_t),
                                               &routing_table_size);
    MDF_ERROR_ASSERT(ret);
    uint8_t ubyCompareEligibilityIndex[routing_table_size];
    bool bReturnFlag = false;
    uint8_t ubyNodeCount = 0;
    for (uint8_t k = 0; k < structRootWrite->ubyNumOfNodes; k++)
    {
        uint8_t ubyAddressIndex = 0;
        memset(ubyCompareEligibilityIndex, 1, routing_table_size * sizeof(uint8_t));
        int iLoopIndex = MWIFI_ADDR_LEN - 1;
        for (int j = MWIFI_ADDR_LEN - 1; j >= 0; j--)
        {
            for (uint8_t i = 1; i < routing_table_size; i++)
            {
                if ((routing_table[i].addr[j] == structRootWrite->ubyNodeMac[j + (k * MWIFI_ADDR_LEN)]) && (ubyCompareEligibilityIndex[i] != 0))
                {
                    ubyCompareEligibilityIndex[i] = i;
                    if (iLoopIndex == j)
                    {
                        ubyAddressIndex++;
                        iLoopIndex--;
                    }
                }
                else
                {
                    ubyCompareEligibilityIndex[i] = 0;
                }
            }
        }
        if (ubyAddressIndex == MWIFI_ADDR_LEN)
        {
            ubyNodeCount++;
        }
    }
    if (ubyNodeCount == structRootWrite->ubyNumOfNodes)
    {
        bReturnFlag = true;
    }
    free(routing_table);
    ESP_LOGI(TAG, "Free Heap After: %d", xPortGetFreeHeapSize());
    return bReturnFlag;
}

esp_err_t RootUtilities_httpEventHandler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}


#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
// GW required changes: function to handle incoming CENCE FW
/**
 * @brief Handles CENCE OTA FW
 *  checks for FW validity, connects to URL and download the FW
 *  copies the FW to the ROOT memory temporarily
 *  Begins sending FW to mainboard
 * @param structRootWrite[in] URL to download the fW from
 */
bool RootUtilities_UpgradeETHRootCenceFirmware(MeshStruct_t *structRootWrite)
{
    // 1. Initialize and reset variables
    static uint8_t ubycount = 0;
    bool success = false;
    mdf_err_t ret = MDF_OK;
    uint8_t *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    char name[32] = {0x0};
    size_t total_size = 0;
    int start_time = 0;
    mupgrade_result_t upgrade_result = {0};
    int status_code = 500;

    // 2. check the validity of the URL
    if ((strncmp(structRootWrite->cReceivedData, "https://", 8) == 0) || (strncmp(structRootWrite->cReceivedData, "http://", 7) == 0))
    {
        MDF_LOGW("Valid URL provided");
    }
    else
    {
        MDF_LOGW("Invalid URL provided, skipping parsing");
        goto NOCLIENT;
    }

    // 3. download the FW from the URL
    // 3.1. create http handler
    esp_http_client_config_t config = {
        .url = structRootWrite->cReceivedData,
        .transport_type = HTTP_TRANSPORT_UNKNOWN,
    };
    // 3.2. connect to  server
    esp_http_client_handle_t client = esp_http_client_init(&config);
    MDF_ERROR_GOTO(!client, EXIT, "Initialise HTTP connection");
    start_time = xTaskGetTickCount();
    ESP_LOGI(TAG, "Open HTTPS connection: %s", structRootWrite->cReceivedData);

    Send_GW_message_to_AWS(64, 0, "Begin OTA");
    // 3.3. establish connection and download the updated firmware
    do
    {
        ret = esp_http_client_open(client, 0);
        if (client == NULL)
        {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection");
            goto EXIT;
        }
        if (ret != MDF_OK)
        {
            MDF_LOGW("<%s> Connection service failed", mdf_err_to_name(ret));
            // Report - connection fail
            if (ubycount == 0)
            {
                // UpdateReportCode(0, AWS_HTTP_CONNECTION_FAILED_WDF);
            }
            ubycount++;
            if (ubycount == 5)
            {
                goto EXIT;
            }
        }
    } while (ret != MDF_OK);
    ESP_LOGI(TAG, "Connected with Server");

    // 3.4. fetch headers and meta data
    total_size = esp_http_client_fetch_headers(client);

    // 3.5. prepare ROOT memory location for the incoming FW data 
    esp_err_t result = GW_ROOT_prepare_memory_to_store_OTA_data_temporarily(total_size);
    if(result != ESP_OK)
    {
        goto EXIT;
    }

    // 3.6. connect to server and copy the FW data to the malloc region
    status_code = esp_http_client_get_status_code(client);
    MDF_LOGW("status code: %d", status_code);
    sscanf(structRootWrite->cReceivedData, "%*[^//]//%*[^/]/%[^.bin]", name);

    if (total_size <= 0 || status_code != HttpStatus_Ok)
    {
        MDF_LOGW("Please check the address of the server");
        ret = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
        MDF_ERROR_GOTO(ret < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));
        MDF_LOGW("Recv data: %.*s", ret, data);
        goto EXIT;
    }

    // 3.7. All the URL checks and data size checks are complete. Now get FW packets chunk by chunk and store it to ROOT's internal memory 
    for (ssize_t size = 0, recv_size = 0; recv_size < total_size; recv_size += size)
    {
        // get FW in chunks
        size = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
        MDF_ERROR_GOTO(size < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));
        if (size > 0)
        {
            // flash the received chunk to the temporary memory location of the ROOT
            result = GW_ROOT_process_OTA_packet(data, size);
            if(result != ESP_OK)
            {
                goto EXIT;
            }
        }
        else
        {
            MDF_LOGW("<%s> esp_http_client_read", mdf_err_to_name(ret));
            goto EXIT;
        }
    }

    // 3.8. generic success message
    MDF_LOGI("The service download firmware is complete, Spent time: %ds", (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);

    // New code here:
    char *start = strstr(structRootWrite->cReceivedData, "cense_");
    char *end = strchr(start, 'b');
    if (start != NULL && end != NULL)
    {
        char extractedString[50];
        strncpy(extractedString, start, end - start);
        extractedString[end - start] = '\0';
        NodeStruct_t *structNodeReceived = malloc(sizeof(NodeStruct_t));
        structNodeReceived->dValue = total_size;
        structNodeReceived->cptrString = extractedString;
        if(GW_Process_OTA_Command_Begin(structNodeReceived))
        {
            ESP_LOGW(TAG, "GWETH begining the FW update task");
            if(GW_ROOT_do_Mainboard_OTA())
            {
                ESP_LOGW(TAG, "FW update done!");
                LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
                Send_GW_message_to_AWS(64, 0, "End OTA");
                success = true;
            }
            else
            {
                ESP_LOGE(TAG, "FW Update ERROR");
                LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
                LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_CN, LED_CHANGE_EXTENDED_DURATION);
                Send_GW_message_to_AWS(65, 0, "End OTA");
            }
        }
        else
        {
            goto EXIT;
        }

        if(restart_ROOT_after_OTA)
        {
            // due to memory issues when acting as both root and eth GW, we do a restart after every OTA attempt
            delayed_ESP_Restart(5000);
        }

    }
    else
    {
        goto EXIT;
    }

EXIT:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
NOCLIENT:
    MDF_FREE(data);
    mupgrade_result_free(&upgrade_result);
    return success;
}
#endif

#if defined(GATEWAY_ETHERNET)
// GW required changes: function to handle incoming CENCE FW
/**
 * @brief Handles CENCE OTA FW to the ROOT
 *  checks for FW validity, connects to URL and download the FW
 *  copies the FW to the ROOT memory temporarily
 *  begins the OTA FW flashing
 * @param structRootWrite[in] URL to download the fW from
 */
bool RootUtilities_UpgradeETHNodeCenceFirmware(MeshStruct_t *structRootWrite)
{
    // 1. Initialize and reset variables
    static uint8_t ubycount = 0;
    bool success = false;
    mdf_err_t ret = MDF_OK;
    uint8_t *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    char name[32] = {0x0};
    size_t total_size = 0;
    int start_time = 0;
    mupgrade_result_t upgrade_result = {0};
    int status_code = 500;

    // 2. check the validity of the URL
    if ((strncmp(structRootWrite->cReceivedData, "https://", 8) == 0) || (strncmp(structRootWrite->cReceivedData, "http://", 7) == 0))
    {
        MDF_LOGW("Valid URL provided");
    }
    else
    {
        MDF_LOGW("Invalid URL provided, skipping parsing");
        goto NOCLIENT;
    }

    // 3. download the FW from the URL
    // 3.1. create http handler
    esp_http_client_config_t config = {
        .url = structRootWrite->cReceivedData,
        .transport_type = HTTP_TRANSPORT_UNKNOWN,
    };
    // 3.2. connect to  server
    esp_http_client_handle_t client = esp_http_client_init(&config);
    MDF_ERROR_GOTO(!client, EXIT, "Initialise HTTP connection");
    start_time = xTaskGetTickCount();
    ESP_LOGI(TAG, "Open HTTPS connection: %s", structRootWrite->cReceivedData);

    // 3.3. establish connection and download the updated firmware
    do
    {
        ret = esp_http_client_open(client, 0);
        if (client == NULL)
        {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection");
            goto EXIT;
        }
        if (ret != MDF_OK)
        {
            MDF_LOGW("<%s> Connection service failed", mdf_err_to_name(ret));
            // Report - connection fail
            if (ubycount == 0)
            {
                // UpdateReportCode(0, AWS_HTTP_CONNECTION_FAILED_WDF);
            }
            ubycount++;
            if (ubycount == 5)
            {
                goto EXIT;
            }
        }
    } while (ret != MDF_OK);
    ESP_LOGI(TAG, "Connected with Server");

    // 3.4. fetch headers and meta data
    total_size = esp_http_client_fetch_headers(client);

    // 3.5. prepare ROOT memory location for the incoming FW data 
    esp_err_t result = GW_ROOT_prepare_memory_to_store_OTA_data_temporarily(total_size);
    if(result != ESP_OK)
    {
        goto EXIT;
    }

    // 3.6. connect to server and copy the FW data to the malloc region
    status_code = esp_http_client_get_status_code(client);
    MDF_LOGW("status code: %d", status_code);
    sscanf(structRootWrite->cReceivedData, "%*[^//]//%*[^/]/%[^.bin]", name);

    if (total_size <= 0 || status_code != HttpStatus_Ok)
    {
        MDF_LOGW("Please check the address of the server");
        ret = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
        MDF_ERROR_GOTO(ret < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));
        MDF_LOGW("Recv data: %.*s", ret, data);
        goto EXIT;
    }

    // 3.7. All the URL checks and data size checks are complete. Now get FW packets chunk by chunk and store it to ROOT's internal memory 
    for (ssize_t size = 0, recv_size = 0; recv_size < total_size; recv_size += size)
    {
        // get FW in chunks
        size = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
        MDF_ERROR_GOTO(size < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));
        if (size > 0)
        {
            // flash the received chunk to the temporary memory location of the ROOT
            result = GW_ROOT_process_OTA_packet(data, size);
            if(result != ESP_OK)
            {
                goto EXIT;
            }
        }
        else
        {
            MDF_LOGW("<%s> esp_http_client_read", mdf_err_to_name(ret));
            goto EXIT;
        }
    }

    // 3.8. generic success message
    MDF_LOGI("The service download firmware is complete, Spent time: %ds", (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);

    // 4. the FW was successfully copied to ROOT's memory. Now, the FW is sent to the target NODE packet by packet
    start_time = xTaskGetTickCount();
    // 4.1. loops over all the target NODEs. most likely this will be just one NODE
    for (uint8_t i = 0; i < structRootWrite->ubyNumOfNodes; i++)
    {
        MDF_LOGI("Sending Firmware to mac add %x:%x:%x:%x:%x:%x", structRootWrite->ubyNodeMac[0 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[1 + (i * MWIFI_ADDR_LEN)],
                 structRootWrite->ubyNodeMac[2 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[3 + (i * MWIFI_ADDR_LEN)],
                 structRootWrite->ubyNodeMac[4 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[5 + (i * MWIFI_ADDR_LEN)]);

        // 4.2. begin packet sending to NODE by getting the begining address of the FW stored in the ROOT 
        // inititialize function variables
        uint32_t ota_agent_core_target_start_address = GW_ROOT_get_OTA_start_address(); 
        int queue_tracker = 0;
        uint32_t ota_agent_core_total_received_data_len = 0;

        // 4.3 find the amount of packets needed for a full transmission
        size_t packet_size = 1024;
        size_t total_packet_count = (total_size + packet_size - 1)/packet_size;
        ESP_LOGI(TAG, "Number of packets of size %d to be sent: %d", packet_size, total_packet_count);

        // 4.4 create the frame for the FW data to be sent
        MeshStruct_t *structRootWriteFW = malloc(sizeof(MeshStruct_t));
        structRootWriteFW->ubyNodeMac = structRootWrite->ubyNodeMac;
        structRootWriteFW->ubyNumOfNodes = 1;

        // 4.5 send packet by packet to the target NODE by iterating over a required amount of time
        for (size_t i = 0; i <= total_packet_count + 1; i++)
        {
            // i=0, this is the first OTA command sent to the NODE. This command includes information about the Target STM32 MCU, the version and the total byte size of the FW
            if (i == 0)
            { 
                // need to notify this is the beginning of the OTA
                // extract the target mcu and the version out of the URL and send to NODE. This will be used by the NODE to identify the target MCU and the incoming FW version
                char *start = strstr(structRootWrite->cReceivedData, "cense_");
                char *end = strchr(start, 'b');
                if (start != NULL && end != NULL)
                {
                    char extractedString[50];
                    strncpy(extractedString, start, end - start);
                    extractedString[end - start] = '\0';

                    RootStruct_t *structRootReceived = malloc(sizeof(RootStruct_t));
                    structRootReceived->ubyCommand = 222; // command of OTA Begin
                    structRootReceived->uwValue = total_size;
                    structRootReceived->cptrString = extractedString;
                    structRootReceived->usrID = ""; 
                    RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWriteFW, false);
                    free(structRootReceived);
                }

                // calculate the wait time needed for NODE to erase it's partitions before writing FW data
                uint32_t time_to_wait = (uint32_t)(0.03 * (float)total_size);
                ESP_LOGW(TAG, "Waiting %dms for NODE to erase the required partitions...", time_to_wait);
                vTaskDelay(pdMS_TO_TICKS(time_to_wait));
            }

            // this is the last OTA command sent to the NODE.
            else if (i == total_packet_count + 1)
            {
                // this command includes information about the total packet count the NODE should have received.
                // need to notify this is the end of the OTA
                RootStruct_t *structRootReceived = malloc(sizeof(RootStruct_t));
                structRootReceived->ubyCommand = 224; // command of OTA End
                structRootReceived->uwValue = total_packet_count;
                structRootReceived->cptrString = "End OTA";
                structRootReceived->usrID = "";
                RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWriteFW, false);
                free(structRootReceived);
            }

            // generic OTA data. This command type has the FW information sent to the NODE
            else
            {    
                // calculate the required packet size to transmit
                size_t read_size = packet_size;
                if (ota_agent_core_total_received_data_len + read_size > total_size)
                {
                    read_size = total_size - ota_agent_core_total_received_data_len;
                }

                // adjust the target address by offseting it from the initial address by the amount of data already sent
                uint32_t target_address = ota_agent_core_target_start_address + ota_agent_core_total_received_data_len;
                ota_agent_core_total_received_data_len += read_size;

                // allocate a dynamic memory buffer to hold the FW data temproarily till it is being sent
                void *read_buffer = malloc(read_size);
                if (read_buffer == NULL)
                {
                    ESP_LOGE(TAG, "Failed to allocate read memory");
                    free(read_buffer);
                    continue;
                }
                else
                {
                    // copy the FW packet from memory using SPI flash read
                    esp_err_t result = spi_flash_read(target_address, read_buffer, read_size);
                    ESP_LOGI(TAG, "Seq: %d,\tData len: %d,\ttotal_read: %d,\ttarget address: 0x%08x,\treading data status: %s", i-1, read_size, ota_agent_core_total_received_data_len, target_address, esp_err_to_name(result));
                    if (result != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Failed to read the FW buffer from memory");
                        free(read_buffer);
                        continue;
                    }
                }
                
                // prepare the buffer to be sent to NODE
                mupgrade_packet_t *packet = MDF_MALLOC(sizeof(mupgrade_packet_t));
                packet->type = MUPGRADE_TYPE_DATA;
                packet->size = read_size;
                packet->seq = i-1;
                memcpy(packet->data, read_buffer, read_size);
                mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_UNICAST, .compression = true};
                mwifi_root_write(structRootWriteFW->ubyNodeMac, structRootWriteFW->ubyNumOfNodes, &data_type, packet,sizeof(mupgrade_packet_t), true);

                // free memory after successful data transmission
                free(read_buffer);
                MDF_FREE(packet);
            }

            // logic used in managing packet sending to NODE
            queue_tracker++;
            if (queue_tracker > 25)
            {
                queue_tracker = 0;
                // We have sent 30 packets at once.
                // We need to wait for some time before sending the other packets so that we are not overflowing the Node receive queue.
                ESP_LOGW(TAG, "Pausing a moment for NODE to handle received data...");
                vTaskDelay(pdMS_TO_TICKS(1500));
            }
        }
        
        // all packets have been sent
        free(structRootWriteFW);
        success = true;
    }

EXIT:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
NOCLIENT:
    MDF_FREE(data);
    mupgrade_result_free(&upgrade_result);
    return success;
}

#endif

#if defined(GATEWAY_SIM7080)
/**
 * @brief Handles CENCE OTA FW targertted to a NODE GW
 *  checks for FW validity, connects to URL and download the FW
 *  copies the FW to the ROOT memory temporarily
 *  sends the FW to NODE
 * @param structRootWrite[in] URL to download the fW from
 */
bool RootUtilities_SIM7080_UpgradeNodeCenceFirmware(MeshStruct_t *structRootWrite)
{
    // 1. Initialize and reset variables
    bool success = false;
    uint8_t *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t total_size = 0;
    int start_time = 0;
    mupgrade_result_t upgrade_result = {0};
    esp_err_t status = ESP_FAIL;
    //char * error_string = "none";
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // 2. check the validity of the URL
    if ((strncmp(structRootWrite->cReceivedData, "https://", 8) == 0) || (strncmp(structRootWrite->cReceivedData, "http://", 7) == 0))
    {
        MDF_LOGW("Valid URL provided");
    }
    else
    {
        MDF_LOGW("Invalid URL provided, skipping parsing");
        goto NOCLIENT;
    }

    LED_assign_task(CNGW_LED_CMD_FW_UPDATE_PRE_PREPARATION, CNGW_LED_COMM);  
        // 4. download the FW from the URL
    status = SIM7080_download_file_to_SIM_from_URL(structRootWrite->cReceivedData);
    if(status != ESP_OK)
    {
        //error_string = "Failed to download binary file to SIM from the given URL";
        goto EXIT;
    }
        // 5. change the state of the LED
    LED_assign_task(CNGW_LED_CMD_FW_UPDATE, CNGW_LED_COMM);

    // 3.4. fetch headers and meta data
    total_size = SIM7080_OTA_data_recieved_length;

    // 3.5. prepare ROOT memory location for the incoming FW data 
    status = GW_ROOT_prepare_memory_to_store_OTA_data_temporarily(total_size);
    if(status != ESP_OK)
    {
        goto EXIT;
    }

    status = SIM7080_copy_bytes_from_SIM_to_ESP(false, 0);
    if(status != ESP_OK)
    {
        //error_string = "Failed to copy bytes from SIM7080 to ESP32";
        goto EXIT;
    }

    // 8. change the state of the LED
    LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);

    // 3.8. generic success message
    MDF_LOGI("The service download firmware is complete, Spent time: %ds", (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);

    // 4. the FW was successfully copied to ROOT's memory. Now, the FW is sent to the target NODE packet by packet
    start_time = xTaskGetTickCount();
    // 4.1. loops over all the target NODEs. most likely this will be just one NODE
    for (uint8_t i = 0; i < structRootWrite->ubyNumOfNodes; i++)
    {
        MDF_LOGI("Sending Firmware to mac add %x:%x:%x:%x:%x:%x", structRootWrite->ubyNodeMac[0 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[1 + (i * MWIFI_ADDR_LEN)],
                 structRootWrite->ubyNodeMac[2 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[3 + (i * MWIFI_ADDR_LEN)],
                 structRootWrite->ubyNodeMac[4 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[5 + (i * MWIFI_ADDR_LEN)]);

        // 4.2. begin packet sending to NODE by getting the begining address of the FW stored in the ROOT 
        // inititialize function variables
        uint32_t ota_agent_core_target_start_address = GW_ROOT_get_OTA_start_address(); 
        int queue_tracker = 0;
        uint32_t ota_agent_core_total_received_data_len = 0;

        // 4.3 find the amount of packets needed for a full transmission
        size_t packet_size = 1024;
        size_t total_packet_count = (total_size + packet_size - 1)/packet_size;
        ESP_LOGI(TAG, "Number of packets of size %d to be sent: %d", packet_size, total_packet_count);

        // 4.4 create the frame for the FW data to be sent
        MeshStruct_t *structRootWriteFW = malloc(sizeof(MeshStruct_t));
        structRootWriteFW->ubyNodeMac = structRootWrite->ubyNodeMac;
        structRootWriteFW->ubyNumOfNodes = 1;

        // 4.5 send packet by packet to the target NODE by iterating over a required amount of time
        for (size_t i = 0; i <= total_packet_count + 1; i++)
        {
            // i=0, this is the first OTA command sent to the NODE. This command includes information about the Target STM32 MCU, the version and the total byte size of the FW
            if (i == 0)
            { 
                // need to notify this is the beginning of the OTA
                // extract the target mcu and the version out of the URL and send to NODE. This will be used by the NODE to identify the target MCU and the incoming FW version
                char *start = strstr(structRootWrite->cReceivedData, "cense_");
                char *end = strchr(start, 'b');
                if (start != NULL && end != NULL)
                {
                    char extractedString[50];
                    strncpy(extractedString, start, end - start);
                    extractedString[end - start] = '\0';

                    RootStruct_t *structRootReceived = malloc(sizeof(RootStruct_t));
                    structRootReceived->ubyCommand = 222; // command of OTA Begin
                    structRootReceived->uwValue = total_size;
                    structRootReceived->cptrString = extractedString;
                    structRootReceived->usrID = ""; 
                    RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWriteFW, false);
                    free(structRootReceived);
                }

                // calculate the wait time needed for NODE to erase it's partitions before writing FW data
                uint32_t time_to_wait = (uint32_t)(0.03 * (float)total_size);
                ESP_LOGW(TAG, "Waiting %dms for NODE to erase the required partitions...", time_to_wait);
                vTaskDelay(pdMS_TO_TICKS(time_to_wait));
            }

            // this is the last OTA command sent to the NODE.
            else if (i == total_packet_count + 1)
            {
                // this command includes information about the total packet count the NODE should have received.
                // need to notify this is the end of the OTA
                RootStruct_t *structRootReceived = malloc(sizeof(RootStruct_t));
                structRootReceived->ubyCommand = 224; // command of OTA End
                structRootReceived->uwValue = total_packet_count;
                structRootReceived->cptrString = "End OTA";
                structRootReceived->usrID = "";
                RootUtilities_PrepareJsonAndSend(structRootReceived, structRootWriteFW, false);
                free(structRootReceived);
            }

            // generic OTA data. This command type has the FW information sent to the NODE
            else
            {    
                // calculate the required packet size to transmit
                size_t read_size = packet_size;
                if (ota_agent_core_total_received_data_len + read_size > total_size)
                {
                    read_size = total_size - ota_agent_core_total_received_data_len;
                }

                // adjust the target address by offseting it from the initial address by the amount of data already sent
                uint32_t target_address = ota_agent_core_target_start_address + ota_agent_core_total_received_data_len;
                ota_agent_core_total_received_data_len += read_size;

                // allocate a dynamic memory buffer to hold the FW data temproarily till it is being sent
                void *read_buffer = malloc(read_size);
                if (read_buffer == NULL)
                {
                    ESP_LOGE(TAG, "Failed to allocate read memory");
                    free(read_buffer);
                    continue;
                }
                else
                {
                    // copy the FW packet from memory using SPI flash read
                    esp_err_t result = spi_flash_read(target_address, read_buffer, read_size);
                    ESP_LOGI(TAG, "Seq: %d,\tData len: %d,\ttotal_read: %d,\ttarget address: 0x%08x,\treading data status: %s", i-1, read_size, ota_agent_core_total_received_data_len, target_address, esp_err_to_name(result));
                    if (result != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Failed to read the FW buffer from memory");
                        free(read_buffer);
                        continue;
                    }
                }
                
                // prepare the buffer to be sent to NODE
                mupgrade_packet_t *packet = MDF_MALLOC(sizeof(mupgrade_packet_t));
                packet->type = MUPGRADE_TYPE_DATA;
                packet->size = read_size;
                packet->seq = i-1;
                memcpy(packet->data, read_buffer, read_size);
                mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_UNICAST, .compression = true};
                mwifi_root_write(structRootWriteFW->ubyNodeMac, structRootWriteFW->ubyNumOfNodes, &data_type, packet,sizeof(mupgrade_packet_t), true);

                // free memory after successful data transmission
                free(read_buffer);
                MDF_FREE(packet);
            }

            // logic used in managing packet sending to NODE
            queue_tracker++;
            if (queue_tracker > 25)
            {
                queue_tracker = 0;
                // We have sent 30 packets at once.
                // We need to wait for some time before sending the other packets so that we are not overflowing the Node receive queue.
                ESP_LOGW(TAG, "Pausing a moment for NODE to handle received data...");
                vTaskDelay(pdMS_TO_TICKS(1500));
            }
        }
        
        // all packets have been sent
        free(structRootWriteFW);
        success = true;
    }

EXIT:
    ESP_LOGW(TAG, "Exiting");
NOCLIENT:
    MDF_FREE(data);
    mupgrade_result_free(&upgrade_result);
    return success;
}

#endif

// TODO@sagar448 #7 This function has a memory leak! Once it executes, it causes other functions to leak memory as well!!!
bool RootUtilities_UpgradeFirmware(MeshStruct_t *structRootWrite, bool toRestart)
{
    static uint8_t ubycount = 0;
    bool success = false;
    mdf_err_t ret = MDF_OK;
    uint8_t *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    char name[32] = {0x0};
    size_t total_size = 0;
    int start_time = 0;
    mupgrade_result_t upgrade_result = {0};
    int status_code = 500;
    // ESP_LOGI(TAG, "This is the string %s", structRootWrite->cReceivedData);
    if ((strncmp(structRootWrite->cReceivedData, "https://", 8) == 0) || (strncmp(structRootWrite->cReceivedData, "http://", 7) == 0))
    {
        MDF_LOGW("Valid URL provided");
    }
    else
    {
        MDF_LOGW("Invalid URL provided, skipping parsing");
        goto NOCLIENT;
    }
    esp_http_client_config_t config = {
        .url = structRootWrite->cReceivedData,
        .transport_type = HTTP_TRANSPORT_UNKNOWN,
    };
    // connect to  server
    esp_http_client_handle_t client = esp_http_client_init(&config);

    MDF_ERROR_GOTO(!client, EXIT, "Initialise HTTP connection");
    start_time = xTaskGetTickCount();
    ESP_LOGI(TAG, "Open HTTPS connection: %s", structRootWrite->cReceivedData);

    // establish connection and download the updated firmware
    do
    {
        ret = esp_http_client_open(client, 0);
        if (client == NULL)
        {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection");
            goto EXIT;
        }
        if (ret != MDF_OK)
        {
            MDF_LOGW("<%s> Connection service failed", mdf_err_to_name(ret));
            // Report - connection fail
            if (ubycount == 0)
            {
                // UpdateReportCode(0, AWS_HTTP_CONNECTION_FAILED_WDF);
                goto EXIT;
            }
            ubycount++;
            if (ubycount == 5)
            {
                goto EXIT;
            }
        }
    } while (ret != MDF_OK);
    ESP_LOGI(TAG, "Connected with Server");

    total_size = esp_http_client_fetch_headers(client);
    status_code = esp_http_client_get_status_code(client);
    MDF_LOGW("status code: %d", status_code);
    sscanf(structRootWrite->cReceivedData, "%*[^//]//%*[^/]/%[^.]", name);

    if (total_size <= 0 || status_code != HttpStatus_Ok)
    {
        MDF_LOGW("Please check the address of the server");
        ret = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
        MDF_ERROR_GOTO(ret < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));
        MDF_LOGW("Recv data: %.*s", ret, data);
        goto EXIT;
    }

    // initialize the upgrade
    ret = mupgrade_firmware_init(name, total_size);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Initialize the upgrade status", mdf_err_to_name(ret));

    // flash the received firmware to the root other partition
    for (ssize_t size = 0, recv_size = 0; recv_size < total_size; recv_size += size)
    {
        size = esp_http_client_read(client, (char *)data, MWIFI_PAYLOAD_LEN);
        MDF_ERROR_GOTO(size < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));
        if (size > 0)
        {
            ret = mupgrade_firmware_download(data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Write firmware to flash, size: %d, data: %.*s",
                           mdf_err_to_name(ret), size, size, data);
        }
        else
        {
            MDF_LOGW("<%s> esp_http_client_read", mdf_err_to_name(ret));
            goto EXIT;
        }
    }

    MDF_LOGI("The service download firmware is complete, Spend time: %ds",
             (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);

    start_time = xTaskGetTickCount();

    MeshStruct_t *structRootFirmwareSend = malloc(sizeof(MeshStruct_t));
    structRootFirmwareSend->ubyNodeMac = malloc(MWIFI_ADDR_LEN * sizeof(uint8_t));
    structRootFirmwareSend->ubyNumOfNodes = 1;

    for (uint8_t i = 0; i < structRootWrite->ubyNumOfNodes; i++)
    {

        // send to all devices
        //  Current leaf node mac address
        MDF_LOGI("Sending Firmware to mac add %x:%x:%x:%x:%x:%x", structRootWrite->ubyNodeMac[0 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[1 + (i * MWIFI_ADDR_LEN)],
                 structRootWrite->ubyNodeMac[2 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[3 + (i * MWIFI_ADDR_LEN)],
                 structRootWrite->ubyNodeMac[4 + (i * MWIFI_ADDR_LEN)], structRootWrite->ubyNodeMac[5 + (i * MWIFI_ADDR_LEN)]);
        ret = mupgrade_firmware_send(&structRootWrite->ubyNodeMac[i * MWIFI_ADDR_LEN], 1, &upgrade_result);
        char deviceMac[18];
        Utilities_MacToString(&structRootWrite->ubyNodeMac[i * MWIFI_ADDR_LEN], deviceMac, 18);
        if (ret != MDF_OK)
        {
            // TODO: Report Firmware Send Error
            // Letting AWS know that the node was updated or not
            RootUtilities_SendStatusUpdate(deviceMac, DEVICE_STATUS_UPDATE_FAIL);
            ESP_LOGE(TAG, "Mupgrade firmware send error <%s>", mdf_err_to_name(ret));
            success = false;
            goto EXITBADNODE;
        }
        success = true;
        RootUtilities_SendStatusUpdate(deviceMac, DEVICE_STATUS_UPDATE_SUCCESS);
        MDF_LOGW("<%s> mupgrade firmware send", mdf_err_to_name(ret));
        if (toRestart)
        {
            // node should be restarted
            memcpy(structRootFirmwareSend->ubyNodeMac, &structRootWrite->ubyNodeMac[0 + (i * MWIFI_ADDR_LEN)], MWIFI_ADDR_LEN);
            RootStruct_t *structRootReceived = malloc(sizeof(RootStruct_t));
            structRootReceived->ubyCommand = 0; // command for RestartNode
            structRootReceived->uwValue = 0;
            structRootReceived->cptrString = "";
            structRootReceived->usrID = ""; // node will not return upon restart, so usrID is not needed for tracking
            RootUtilities_PrepareJsonAndSend(structRootReceived, structRootFirmwareSend, false);
            // RootUtilities_PrepareJsonAndSend(0, 0, cptrStr, structRootFirmwareSend, usrID);
            free(structRootReceived);
        }
    }
EXITBADNODE:
    MDF_LOGI("Firmware is sent to the device to complete, Spend time: %ds",
             (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);
    MDF_LOGI("Devices upgrade completed, successed_num: %d, unfinished_num: %d", upgrade_result.successed_num, upgrade_result.unfinished_num);
    free(structRootFirmwareSend->ubyNodeMac);
    free(structRootFirmwareSend);

EXIT:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
NOCLIENT:
    MDF_FREE(data);
    mupgrade_result_free(&upgrade_result);
    return success;
}

bool RootUtilities_PrepareJsonAndSend(RootStruct_t *structRootReceived, MeshStruct_t *structRootWrite, bool isGroupFunction)
{
    cJSON *response = cJSON_CreateObject();
    cJSON *cjCmnd = cJSON_CreateNumber((uint16_t)structRootReceived->ubyCommand);
    cJSON_AddItemToObject(response, "cmnd", cjCmnd);
    cJSON_AddNumberToObject(response, "val", structRootReceived->uwValue);
    cJSON_AddItemToObject(response, "str", cJSON_CreateString(structRootReceived->cptrString));
    cJSON_AddItemToObject(response, "usrID", cJSON_CreateString(structRootReceived->usrID));
    structRootWrite->cReceivedData = cJSON_PrintUnformatted(response);
    if (isGroupFunction)
    {
        if (!RootUtilities_SendDataToGroup(structRootWrite))
            return false;
    }
    else
    {
        if (!RootUtilities_RootWrite(structRootWrite))
            return false;
    }
    free(structRootWrite->cReceivedData);
    cJSON_Delete(response);
    return true;
}

bool RootUtilities_RootWrite(MeshStruct_t *structRootWrite)
{
    mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_UNICAST, .compression = true};
    ESP_LOGI(TAG, "Data to Send: %s, len: %d", structRootWrite->cReceivedData, strlen(structRootWrite->cReceivedData));
    mdf_err_t ret = mwifi_root_write(structRootWrite->ubyNodeMac, structRootWrite->ubyNumOfNodes, &data_type, structRootWrite->cReceivedData, strlen(structRootWrite->cReceivedData) + 1, true);
    if (ret == MDF_OK)
    {
        ESP_LOGI(TAG, "Data sent! Free heap on Root: %u", esp_get_free_heap_size());
        return true;
    }
    else
    {
        // Report back to a error topic saying could not send
        ESP_LOGE(TAG, "Error %s", mdf_err_to_name(ret));
        return false;
    }
}

uint8_t *RootUtilities_ParseMacAddress(cJSON *cjMacs, uint8_t *ubyptrNodeMacs)
{
    cJSON *Iterator = NULL;
    uint8_t count = 0;
    cJSON_ArrayForEach(Iterator, cjMacs)
    {
        if (cJSON_IsNumber(Iterator) && Iterator->valueint >= 0x00 && Iterator->valueint <= 0xff)
        {
            ubyptrNodeMacs[count] = (uint8_t)Iterator->valueint;
            count++;
        }
        else
        {
            ESP_LOGI(TAG, "invalid mac address pair detected");
            return NULL;
        }
    }
    return ubyptrNodeMacs;
}

bool RootUtilities_IsCommandFromNode(int cmnd)
{
    for (int i = 0; i < NUM_ROOT_COMMANDS_FROM_NODE; i++)
    {
        if (cmnd == ROOT_COMMANDS_FROM_NODE[i])
        {
            return true;
        }
    }
    return false;
}

#endif