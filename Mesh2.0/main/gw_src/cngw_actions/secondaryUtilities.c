/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: secondaryUtilities.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: codes involved in GW processing the incoming messages just as it will if it is a IPNODE.
 * The following code is intelded ONLY for GATEWAY_SIM7080 or GATEWAY_ETH modes
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_SIM7080) || defined(GATEWAY_ETH)
#include "gw_includes/secondaryUtilities.h"
static const char *TAG = "sec_utilities";
#ifdef GW_DEBUGGING
static bool Print_Info = true;
#else
static bool Print_Info = false;
#endif

#ifdef GATEWAY_SIM7080

/**
 * @brief helper function to obtain valueInt from cjVals to arrvals
 */
static void SIM7080_ParseArrVal(cJSON *cjVals, uint8_t *arrvals)
{
    cJSON *Iterator = NULL;
    uint8_t count = 0;
    cJSON_ArrayForEach(Iterator, cjVals)
    {
        if (cJSON_IsNumber(Iterator))
        {
            arrvals[count] = (uint8_t)Iterator->valueint;
            count++;
        }
    }
}

/**
 * @brief Task which handles sending response to AWS with SIM7080 interface
 * loops continuously. if there is a message in the queue SIM7080_AWS_Tx_queue, then:
 * gets the message from the queue
 * create a NodeStruct_t struct out of it
 * call function SIM7080_send_AWS_message based on whether it is a success ort a fail message
 */
void SIM7080_AWS_Tx_task(void *pvParameters)
{
    char *pBuffer = NULL;
    while (true)
    {
        // check if there is any messages in the queue
        if (SIM7080_AWS_Tx_queue != NULL)
        {
            // get the message to the pBuffer mem location
            if (xQueueReceive(SIM7080_AWS_Tx_queue, &pBuffer, (TickType_t)0))
            {
                // create the struct
                NodeStruct_t *structNodeReceived = malloc(sizeof(NodeStruct_t));
                cJSON *json = cJSON_Parse(pBuffer);
                cJSON *cjCommand = cJSON_GetObjectItemCaseSensitive(json, "cmnd");

                // check for validity of the contents
                if (cJSON_IsNumber(cjCommand))
                {
                    // if it is a success message
                    if (cjCommand->valueint == 64)
                    {
                        if (Print_Info)
                        {
                            ESP_LOGI(TAG, "Sending SUCCESS message to AWS: %s", pBuffer);
                        }

                        // only send the message if it is within the valid size
                        if (strlen(pBuffer) <= AWS_Tx_BUFFER_SIZE)
                        {
                            SIM7080_send_AWS_message(pBuffer, true);
                        }
                        else
                        {
                            if(Print_Info)
                            {
                                ESP_LOGE(TAG, "Tx buffer size exceeds the allowed limit");
                            }

                            SIM7080_send_AWS_message("{\nTx buffer size exceeds the allowed limit\n}", false);
                        }
                    }

                    // if it is a fail message
                    else if (cjCommand->valueint == 65)
                    {
                        if (Print_Info)
                        {
                            ESP_LOGI(TAG, "Sending FAIL message to AWS: %s", pBuffer);
                        }

                        // only send the message if it is within the valid size
                        if (strlen(pBuffer) <= AWS_Tx_BUFFER_SIZE)
                        {
                            SIM7080_send_AWS_message(pBuffer, false);
                        }
                        else
                        {
                            if(Print_Info)
                            {
                                ESP_LOGE(TAG, "Tx buffer size exceeds the allowed limit");
                            }
                            SIM7080_send_AWS_message("{\nTx buffer size exceeds the allowed limit\n}", false);
                        }
                    }
                }
                
                cJSON_Delete(json);
                free(structNodeReceived);
                vPortFree(pBuffer);
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); // Leave this for watchdog
    }
}


/**
 * @brief Task which handles sending response to AWS with SIM7080 interface
 * 
 * NOTE: The task is initiated in SIM7080.c : init_SIM7080 function. The queue SIM7080_AWS_Rx_queue is
 * filled ONLY by the SIM7080 module, (for MQTT messages coming to SIM7080_msg_preprocessor function).
 * Also, Messages are NOT added to this queue IF IPNODE is defined. (for GW acting as IPNODE + SIM7080),
 * the MQTT messages coming to SIM is also added to the IPNODE queue and executed from there.
 * 
 */
void SIM7080_CommandExecutionTask(void *arg)
{
    char *cptrNodeData = NULL;
    bool isArray = false;
    while (true)
    {
        if (SIM7080_AWS_Rx_queue != NULL)
        {
            if (xQueueReceive(SIM7080_AWS_Rx_queue, &cptrNodeData, (TickType_t)0))
            {
                NodeStruct_t *structNodeReceived = malloc(sizeof(NodeStruct_t));
                cJSON *json = cJSON_Parse(cptrNodeData);

                if (json == NULL)
                {
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != NULL)
                    {
                        ESP_LOGE(TAG, "Error while parsing SIM7080_CommandExecutionTask");
                    }
                    SecondaryUtilities_PrepareJSONAndSendToAWS(65, 0, cptrNodeData);
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
                                SIM7080_ParseArrVal(cjValue, structNodeReceived->arrValues);
                            }
                            cJSON *cjString = cJSON_GetObjectItemCaseSensitive(json, "str");
                            if (cJSON_IsString(cjString))
                            {
                                structNodeReceived->cptrString = cjString->valuestring;
                                bool returnVal = SecondaryUtilities_ValidateAndExecuteCommand(structNodeReceived);
                                if (returnVal)
                                {
                                    SecondaryUtilities_PrepareJSONAndSendToAWS(64, 0, cptrNodeData);
                                }
                                else
                                {
                                    SecondaryUtilities_PrepareJSONAndSendToAWS(65, 0, cptrNodeData);
                                }
                                goto NodeOperations_CommandExecutionTask_End;
                            }
                        }
                    }

                        SecondaryUtilities_PrepareJSONAndSendToAWS(65, 0, cptrNodeData);
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

#endif


/**
 * @brief Task which handles sending response to AWS with SIM7080 interface and also with GATEWAY_ETH interface

 * @return ESP_OK if OTA partition found, else ESP_FAIL
 */
void SecondaryUtilities_PrepareJSONAndSendToAWS(uint16_t ubyCommand, uint32_t uwValue, char *cptrString)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "cmnd", ubyCommand);
    cJSON_AddNumberToObject(response, "val", uwValue);
    cJSON_AddItemToObject(response, "str", cJSON_CreateString(cptrString));
    char *dataToSend = cJSON_PrintUnformatted(response);
    char *payload = (char *)pvPortMalloc((strlen(dataToSend) + 1) * sizeof(char));
    memset(payload, 0, strlen(dataToSend) + 1);
    strcpy(payload, dataToSend);
    
#ifdef GATEWAY_SIM7080
    if (SIM7080_AWS_Tx_queue != NULL)
    {
        if (xQueueSendToBack(SIM7080_AWS_Tx_queue, &payload, (TickType_t)0) != pdPASS)
        {
            if(Print_Info)
            {
                ESP_LOGE(TAG, "SIM7080_AWS_Tx_queue Queue is full");
            }
            vPortFree(payload);
        }
    }
#else
    if(ubyCommand == 64)
    {
        RootUtilities_SendDataToAWS(controlLogTopicSuccess, payload);
    }
    else
    {
        RootUtilities_SendDataToAWS(controlLogTopicFail, payload);
    }

    vPortFree(payload);
#endif
    free(dataToSend);
    cJSON_Delete(response);
}

bool SecondaryUtilities_ValidateAndExecuteCommand(NodeStruct_t *structNodeReceived)
{
    bool result = false;
    
    if (Print_Info)
    {
        ESP_LOGI(TAG, "Command Received: %d", structNodeReceived->ubyCommand);
    }

    switch(structNodeReceived->ubyCommand)
    {
#ifndef IPNODE
#ifdef GATEWAY_SIM7080
        // only for a GW defined as SIM7080
        case 61:
        {
            // OTA command
            result = SIM7080_UpdateNodeFW(structNodeReceived);

        }break;
#endif
#endif
        case 220:
        {
            //action command
            result = GW_Process_Action_Command(structNodeReceived);
        }
        break;
        case 221:
        {
            //query command
            result = GW_Process_Query_Command(structNodeReceived);
        }
        break;

        case 222:
        {
            //OTA begin command (for NODE)
            result = false;
        }
        break;

        case 224:
        {
            //OTA END command (for NODE)
            result = false;
        }
        break;

        case 225:
        {
            //AT command
            result = GW_Process_AT_Command(structNodeReceived);
        }
        break;
        default:
        {
            ESP_LOGE(TAG, "Incorrect Command Received.");
        }
        break;
    }

    return result;
}

#endif