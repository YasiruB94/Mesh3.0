// Copyright Argentum Electronics Inc.
//To track changes done for CENCE GW: "GW required changes"
//To track temp changes: "temp changes"

#include "includes/mesh_connection.h"
#include "includes/node_utilities.h"
#include "includes/esp_now_utilities.h"
#include "includes/node_operations.h"
#include "includes/root_operations.h"
#include "includes/aws.h"

// load init_GW for the below two cases
#if defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
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
    RootUtilities_loadOrgInfo();
    xTaskCreate(AWS_AWSTask, "aws_task", 5120, NULL, 4, NULL);
    xTaskCreate(RootOperations_RootGroupReadTask, "RootGroupReadTask", 2048, NULL, 4, NULL);
    xTaskCreate(RootOperations_ProcessNodeCommands, "ProcessNodeCmnds", 4096, NULL, 4, NULL);
    xTaskCreate(RootOperations_ProcessRootCommands, "ProcessRootCmnds", 4096, NULL, 5, NULL);
    xTaskCreate(RootOperations_RootReadTask, "RootReadTask", 4096, NULL, 4, NULL);
}
#endif

#ifdef IPNODE
void NodeTasks()
{
    NodeUtilities_WhoAmI();
    NodeUtilities_InitialiseAndSetNodeOutput();
    NodeUtilities_LoadAllNodeGroups();
    EspNow_Init();
    EspNow_CreateEspNowQueue();
    xTaskCreate(NodeOperations_NodeReadTask, "NodeOperations_NodeReadTask", 4096, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    xTaskCreate(NodeOperations_RootSendTask, "NodeOperations_RootSendTask", 4096, NULL, 4, NULL);
    xTaskCreate(NodeOperations_CommandExecutionTask, "NodeOperations_CommandExecutionTask", 5120, NULL, 4, NULL);
    xTaskCreate(NodeOperations_ProcessEspNowCommands, "process_espnow_commands", 4096, NULL, 4, NULL);

}
#endif

void app_main()
{
    //This is cool you can set verbosity based on TAG
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    // need queues here for the mesh setup
    InitisaliseMisc();

// device is configured as a root (can be a GW ETH)
#ifdef ROOT
#ifdef GATEWAY_ETH
    // if defined ETH_GW, we need to initialize the I2C first. hence, call initialize_GW first
    Initialize_Gateway();
#endif
    RootUtilities_CreateQueues();
    AWS_CreateTopics();
    MeshConnection_MeshSetup();
    xEventGroupWaitBits(ethernet_event_group, ethernet_bit, false, true, portMAX_DELAY);
    RootTasks();
#endif

// device is configured as a node (can be a GW SIM)
#ifdef IPNODE
    NodeUtilities_CreateQueues();
    MeshConnection_MeshSetup();
    xEventGroupWaitBits(ethernet_event_group, ethernet_bit, false, true, portMAX_DELAY);
    NodeTasks();
#else

#ifdef GATEWAY_SIM7080
    // device is configured only as a GW SIM
    Initialize_Gateway();
#endif
#endif


    /* Periodic print system information */

    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS, true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);
}