#ifdef IPNODE
#ifndef NODE_UTILITIES_H
#define NODE_UTILITIES_H

#include "utilities.h"
#include "node_commands.h"
#include "espnow_ota.h"
//#include "SpacrGateway_commands.h"

/************ BASE NODE COMMANDS ************/
enum enumNodeCmndKey
{
    enumNodeCmndKey_RestartNode                 = 0,
    enumNodeCmndKey_TestNode                    = 1,
    enumNodeCmndKey_SetNodeOutput               = 2,
    enumNodeCmndKey_CreateGroup                 = 3,
    enumNodeCmndKey_DeleteGroup                 = 4,
    enumNodeCmndKey_AssignSensorToSelf          = 5,
    enumNodeCmndKey_EnableOrDisableSensorAction = 6,
    enumNodeCmndKey_RemoveSensor                = 7,
    enumNodeCmndKey_AssignSensorToNodeOrGroup   = 8,
    enumNodeCmndKey_ChangePortPIR               = 9,
    enumNodeCmndKey_OverrideNodeType            = 10,
    enumNodeCmndKey_RemoveNodeTypeOverride      = 11,
    enumNodeCmndKey_GetDaisyChainedCurrent      = 12,
    enumNodeCmndKey_SendFirmwareVersion         = 13,
    enumNodeCmndKey_GetNodeOutput               = 14,
    enumNodeCmndKey_GetParentRSSI               = 15,
    enumNodeCmndKey_GetNodeChildren             = 16,
    enumNodeCmndKey_SetAutoSensorPairing        = 17,
    enumNodeCmndKey_SetSensorMeshID             = 18,
    enumNodeCmndKey_SendSensorFirmware          = 19,
    enumNodeCmndKey_ClearSensorCommand          = 20,
    enumNodeCmndKey_ClearNVS                    = 21,
    enumNodeCmndKey_TotalNumOfCommands          = 23
};

#ifdef GATEWAY_IPNODE
/************ GATEWAY NODE COMMANDS ************/
enum enumGWCmndKey
{
    enumGatewayCmndKey_GW_Action                        = 220,
    enumGatewayCmndKey_GW_Query                         = 221,
    enumGatewayCmndKey_GW_GW_ROOT_find_OTA_partition    = 222,
    enumGatewayCmndKey_GW_GW_ROOT_OTA_end               = 224,
    enumGatewayCmndKey_GW_AT_CMD                        = 225,
    enumGatewayCmndKey_TotalNumOfCommands               = 6
};

#define GATEWAY_NODE_COMMAND_RANGE_STARTS   220
#endif

#define COMMAND_VALUE_SPAN                  50
#define BASE_NODE_COMMAND_RANGE_STARTS      0        
#define NODE_OUTPUT_PIN_MASk                (1ULL << NODE_OUTPUT_PIN_DEFAULT)

extern uint8_t devType;
extern uint8_t autoSensorPairing;
extern QueueHandle_t nodeReadQueue;
extern QueueHandle_t rootSendQueue;
extern size_t firmware_size;
extern uint8_t sha_256[32];
extern char *dataPointsGlobal[30];
extern uint8_t numberOfDataPoints;
extern bool rootReachable;

void NodeUtilities_overrideNodeType(uint8_t nodeType);
#ifdef GATEWAY_IPNODE
void NodeUtilities_initiateGatewayNode();
#endif
void NodeUtilities_initiateNothingNode();
extern void NodeUtilities_WhoAmI();
extern void NodeUtilities_PrepareJsonAndSendToRoot(uint16_t ubyCommand, uint32_t uwValue, char *cptrString);
extern bool NodeUtilities_LoadGroup();
extern void NodeUtilities_PrepareAndSendRegisterData();
extern void NodeUtilities_CreateQueues();
extern bool NodeUtilities_ValidateAndExecuteCommand(NodeStruct_t *structNodeReceived);
extern bool NodeUtilities_compareWithGroupMac(uint8_t *mac);
extern void NodeUtilities_InitGpios(uint64_t mask, gpio_mode_t mode);
extern adc1_channel_t NodeUtilities_AdcChannelLookup(uint8_t gpio);
extern uint32_t NodeUtilities_ReadMilliVolts(uint8_t gpio, uint16_t samples, esp_adc_cal_characteristics_t *adc_chars);
extern void NodeUtilities_InitialiseAndSetNodeOutput();
extern bool NodeUtilities_GetAutoPairing();
extern bool NodeUtilities_LoadAndDeleteGroup(nvs_handle nvsHandle);
void NodeUtilities_SendDataToGroup(uint8_t *ubyGroupMacAddr, char *cptrDataToSend);
extern void NodeUtilities_SendToNode(uint8_t *macOfNode, char *dataToSend);
extern void NodeUtilities_ParseArrVal(cJSON *cjVals, double *arrvals);
extern void NodeUtilities_SendFirmware(size_t firmware_size, uint8_t sha[ESPNOW_OTA_HASH_LEN], uint8_t dest_addr[6]);
extern esp_err_t ota_initiator_data_cb(size_t src_offset, void *dst, size_t size);
#endif
#endif