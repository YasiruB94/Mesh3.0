#ifdef IPNODE
#ifndef NODE_UTILITIES_H
#define NODE_UTILITIES_H

#include "utilities.h"
#include "node_commands.h"
#include "SpacrIO_commands.h"
#include "SpacrPower_commands.h"
#include "SpacrContactor_commands.h"
//#include "espnow_ota.h"
//GW required changes: include the GW header file
#include "SpacrGateway_commands.h"

enum enumNodeCmndKey
{
    /************ BASE NODE COMMANDS ************/
    enumNodeCmndKey_RestartNode = 0,
    enumNodeCmndKey_TestNode = 1,
    enumNodeCmndKey_SetNodeOutput = 2,
    enumNodeCmndKey_CreateGroup = 3,
    enumNodeCmndKey_DeleteGroup = 4,
    enumNodeCmndKey_AssignSensorToSelf = 5,
    enumNodeCmndKey_EnableOrDisableSensorAction = 6,
    enumNodeCmndKey_RemoveSensor = 7,
    enumNodeCmndKey_AssignSensorToNodeOrGroup = 8,
    enumNodeCmndKey_ChangePortPIR = 9,
    enumNodeCmndKey_OverrideNodeType = 10,
    enumNodeCmndKey_RemoveNodeTypeOverride = 11,
    enumNodeCmndKey_GetDaisyChainedCurrent = 12,
    enumNodeCmndKey_GetFirmwareVersion = 13,
    enumNodeCmndKey_GetNodeOutput = 14,
    enumNodeCmndKey_GetParentRSSI = 15,
    enumNodeCmndKey_GetNodeChildren = 16,
    enumNodeCmndKey_TotalNumOfCommands = 18

};
enum enumContactorCmndKey
{
    /************ CONTACTOR NODE COMMANDS ************/
    enumContactorCmndKey_SC_SetRelays = 100,
    enumContactorCmndKey_SC_SelectADCs = 101,
    enumContactorCmndKey_SC_SetADCPeriod = 102,
    enumContactorCmndKey_TotalNumOfCommands = 4
};

enum enumIOCmndKey
{
    /************ IO NODE COMMANDS ************/
    enumIOCmndKey_SIO_SetVoltage = 150,
    enumIOCmndKey_SIO_SelectADCs = 151,
    enumIOCmndKey_SIO_SetADCPeriod = 152,
    enumIOCmndKey_SIO_EnableDisableSub = 153,
    enumIOCmndKey_SIO_SetRelay = 154,
    enumIOCmndKey_TotalNumOfCommands = 6
};

enum enumPowerCmndKey
{
    /************ POWER NODE COMMANDS ************/
    enumPowerCmndKey_SP_SetVoltage = 200,
    enumPowerCmndKey_SP_SetPWMPort0 = 201,
    enumPowerCmndKey_SP_SetPWMPort1 = 202,
    enumPowerCmndKey_SP_SetPWMBothPort = 203,
    enumPowerCmndKey_SP_EnableCurrentSubmission = 204,
    enumPowerCmndKey_TotalNumOfCommands = 6
};

//GW required changes: define the GW node commands here
enum enumGWCmndKey
{
    /************ GATEWAY NODE COMMANDS ************/
    enumGatewayCmndKey_GW_Action = 220,
    enumGatewayCmndKey_GW_Query = 221,
    enumGatewayCmndKey_GW_OTA_Begin = 222,
    enumGatewayCmndKey_GW_OTA_End = 224,
    enumGatewayCmndKey_GW_AT_CMD = 225,
    enumGatewayCmndKey_TotalNumOfCommands = 6
};

//GW required changes: GW commands starts from number 220
#define GATEWAY_NODE_COMMAND_RANGE_STARTS 220
#define POWER_NODE_COMMAND_RANGE_STARTS 200
#define IO_NODE_COMMAND_RANGE_STARTS 150
#define CONTACTOR_NODE_COMMAND_RANGE_STARTS 100
#define COMMAND_VALUE_SPAN 50
#define BASE_NODE_COMMAND_RANGE_STARTS 0

extern uint8_t nodeOutputPin;
extern uint8_t devType;
extern QueueHandle_t nodeReadQueue;
extern QueueHandle_t rootSendQueue;

void NodeUtilities_overrideNodeType(uint8_t nodeType);
void NodeUtilities_initiatePowerNode();
void NodeUtilities_initiateSpacrIO();
void NodeUtilities_initiateContactor();
//GW required changes
void NodeUtilities_initiateGatewayNode();
extern void NodeUtilities_WhoAmI();
extern void NodeUtilities_PrepareJsonAndSendToRoot(uint16_t ubyCommand, uint32_t uwValue, char *cptrString);
extern bool NodeUtilities_LoadAllNodeGroups();
extern void NodeUtilities_CreateQueues();
extern bool NodeUtilities_ValidateAndExecuteCommand(NodeStruct_t *structNodeReceived);
extern bool NodeUtilities_compareWithGroupMac(uint8_t *mac);
extern void NodeUtilities_InitGpios(uint64_t mask, gpio_mode_t mode);
extern adc1_channel_t NodeUtilities_AdcChannelLookup(uint8_t gpio);
extern uint32_t NodeUtilities_ReadMilliVolts(uint8_t gpio, uint8_t samples, esp_adc_cal_characteristics_t *adc_chars);
extern void NodeUtilities_InitialiseAndSetNodeOutput();
void NodeUtilities_SendDataToGroup(uint8_t *ubyGroupMacAddr, char *cptrDataToSend);
extern void NodeUtilities_SendToNode(uint8_t *macOfNode, char *dataToSend);
extern void NodeUtilities_ParseArrVal(cJSON *cjVals, uint8_t *arrvals);

#endif
#endif