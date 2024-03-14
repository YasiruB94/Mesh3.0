#ifdef IPNODE
#ifndef NODE_COMMANDS_H
#define NODE_COMMANDS_H
#include "utilities.h"

typedef struct
{
    uint8_t ubyCommand;
    double dValue;
    uint8_t *arrValues;
    uint8_t arrValueSize;
    char *cptrString;
} NodeStruct_t;

typedef struct
{
    uint8_t ubyKey;
    bool (*node_fun)(NodeStruct_t *structNodeReceived);
} NodeCmnds_t;

extern bool RestartNode(NodeStruct_t *structNodeReceived);
extern bool TestNode(NodeStruct_t *structNodeReceived);
extern bool SetNodeOutput(NodeStruct_t *structNodeReceived);
extern bool CreateGroup(NodeStruct_t *structNodeReceived);
extern bool DeleteGroup(NodeStruct_t *structNodeReceived);
extern bool AssignSensorToSelf(NodeStruct_t *structNodeReceived);
extern bool EnableOrDisableSensorAction(NodeStruct_t *structNodeReceived);
extern bool AssignSensorToNodeOrGroup(NodeStruct_t *structNodeReceived);
extern bool RemoveSensor(NodeStruct_t *structNodeReceived);
extern bool ChangePortPIR(NodeStruct_t *structNodeReceived);
extern bool OverrideNodeType(NodeStruct_t *structNodeReceived);
extern bool RemoveNodeTypeOverride(NodeStruct_t *structNodeReceived);
extern bool GetDaisyChainedCurrent(NodeStruct_t *structNodeReceived);
extern bool GetFirmwareVersion(NodeStruct_t *structNodeReceived);
extern bool GetNodeOutput(NodeStruct_t *structNodeReceived);
extern bool GetParentRSSI(NodeStruct_t *structNodeReceived);
bool GetNodeChildren(NodeStruct_t *structNodeReceived);

#endif
#endif