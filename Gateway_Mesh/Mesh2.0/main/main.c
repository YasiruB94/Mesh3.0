// Copyright Argentum Electronics Inc.
//To track changes done for CENCE GW: "GW required changes"

#include "includes/mesh_connection.h"
#include "includes/node_utilities.h"
#include "includes/esp_now_utilities.h"
#include "includes/node_operations.h"
#include "includes/root_operations.h"
#include "includes/aws.h"

//GW required changes: load Initialize_Gateway for the below two cases
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#endif

static const char *TAG = "Main";

//*******Program wide global variables******
char deviceMACStr[18] = "";

//*******Program wide global variables******
void InitisaliseMisc()
{
    Utilities_InitializeNVS();
    Utilities_GetDeviceMac(deviceMACStr);
}

#ifdef ROOT
void RootTasks()
{
#ifndef GATEWAY_SIM7080
    xTaskCreate(AWS_AWSTask, "aws_task", 5120, NULL, 4, NULL);
#endif
    xTaskCreate(RootOperations_RootGroupReadTask, "RootGroupReadTask", 2048, NULL, 4, NULL);
    xTaskCreate(RootOperations_ProcessNodeCommands, "ProcessNodeCmnds", 4096, NULL, 4, NULL);
    xTaskCreate(RootOperations_ProcessRootCommands, "ProcessRootCmnds", 5500, NULL, 5, NULL);
    xTaskCreate(RootOperations_RootReadTask, "RootReadTask", 4096, NULL, 4, NULL);
}
#else
void NodeTasks()
{
    NodeUtilities_WhoAmI();
    NodeUtilities_InitialiseAndSetNodeOutput();
    NodeUtilities_LoadGroup();
    NodeUtilities_GetAutoPairing();

    xTaskCreate(NodeOperations_NodeReadTask, "NodeOperations_NodeReadTask", 4096, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    xTaskCreate(NodeOperations_RootSendTask, "NodeOperations_RootSendTask", 4096, NULL, 4, NULL);
    xTaskCreate(NodeOperations_CommandExecutionTask, "NodeOperations_CommandExecutionTask", 5120, NULL, 4, NULL);
    xTaskCreate(NodeOperations_ProcessEspNowCommands, "process_espnow_commands", 4096, NULL, 4, NULL);
    xTaskCreate(NodeOperations_ReadFromSensorTask, "NodeOperations_ReadFromSensorTask", 4096, NULL, 4, NULL);

    // Sending the device information to register topic
    NodeUtilities_PrepareAndSendRegisterData();
}
#endif

void app_main()
{
    // This is cool you can set verbosity based on TAG
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    // need queues here for the mesh setup

    // these values defined in the CMakeLists
    ESP_LOGI(TAG, "GIT BRANCH IS %s, hash %s committed at %s", GIT_BRANCH, GIT_HASH, GIT_TIME);
    //ESP_LOGW(TAG, "########################## GATEWAY_ETHERNET CODE FROM AWS ########################");
    InitisaliseMisc();
#ifdef ROOT
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
    // The ROOT is also a Gateway. Therefore, we need to initialize the I2C first. (wakeup pin of PHY ETH is controlled by the LED driver) hence, call initialize_GW first
    Initialize_Gateway();
#endif
    RootUtilities_CreateQueues();
    RootUtilities_loadOrgInfo();
    AWS_CreateTopics();
#else
    NodeUtilities_CreateQueues();
    EspNow_CreateEspNowQueue();
#endif
    MeshConnection_MeshSetup();

#ifndef GATEWAY_SIM7080
    // for GATEWAY_SIM7080, there is no ethernet internet connection. therefore we do not wait for the below bit to be set 
#ifdef GATEWAY_ETHERNET
        LED_assign_task(CNGW_LED_CMD_CONN_PENDING, CNGW_LED_COMM);
#endif
    xEventGroupWaitBits(ethernet_event_group, ethernet_bit, false, true, portMAX_DELAY);
#ifdef GATEWAY_ETHERNET
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
#endif
#endif

#ifdef ROOT
    RootTasks();
#else
    NodeTasks();
#endif

    /* Periodic print system information */

    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS, true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);
}