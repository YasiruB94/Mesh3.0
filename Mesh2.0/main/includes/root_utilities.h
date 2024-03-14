#ifdef ROOT

#ifndef _ROOTUTILITIES_H_
#define _ROOTUTILITIES_H_

#include "utilities.h"
#include "root_commands.h"
#include "esp_https_ota.h"

extern char orgID[25];
extern char *logTopic;
extern char *sensorLogTopic;
extern char *controlLogTopic;
extern char *controlLogTopicSuccess;
extern char *controlLogTopicFail;

#define COMMAND_VALUE_SPAN 50
#define ROOT_NODE_COMMAND_RANGE_STARTS 50

typedef struct
{
   uint8_t ubyKey;
   uint8_t (*root_fun)(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);
} RootCmnds_t;

typedef struct
{
   char *cptrTopic;
   char *cptrPayload;
} AWSPub_t;
// TODO: #55 @sagar448 @cambrian-dk struct for passing around data is unnecessary, can be removed
typedef struct
{
   char *cReceivedData;
   uint8_t *ubyNodeMac;
   uint8_t ubyNumOfNodes;
} MeshStruct_t;

extern void RootUtilities_loadOrgInfo();
extern void RootUtilities_CreateQueues();
extern bool RootUtilities_ParseNodeAddressAndData(MeshStruct_t *structRootWrite, char *cptrRcvdData);
extern void RootUtilities_SendDataToAWS(char *cptrTopic, char *cptrPayload);
extern void RootUtilities_PrepareJsonAndSendDataToGroup(uint16_t ubyCommand, uint32_t uwValue, char *cptrString, MeshStruct_t *structRootWrite);
uint8_t RootUtilities_ValidateAndExecuteCommand(uint8_t ubyCommand, uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);
extern uint8_t *RootUtilities_ParseMacAddress(cJSON *cjMacs, uint8_t *ubyptrNodeMacs);
extern esp_err_t RootUtilities_httpEventHandler(esp_http_client_event_t *evt);
extern bool RootUtilities_SendStatusUpdate(char *devID, uint8_t status);
extern bool RootUtilities_RootWrite(MeshStruct_t *structRootWrite);
extern bool RootUtilities_NodeAddressVerification(MeshStruct_t *structRootWrite);
extern bool RootUtilities_PrepareJsonAndSend(uint16_t ubyCommand, uint32_t uwValue, char *cptrString, MeshStruct_t *structRootWrite);
extern void RootUtilities_SendDataToGroup(MeshStruct_t *structRootWrite);
extern void RootUtilities_UpgradeNodeFirmware(MeshStruct_t *structReceived);
//GW required changes: define the function
extern void RootUtilities_UpgradeNodeCenceFirmware(MeshStruct_t *structRootWrite);
#ifdef GATEWAY_ETH
extern void RootUtilities_ETHROOTUpgradeNodeCenceFirmware(MeshStruct_t *structRootWrite);
#endif

#endif
#endif