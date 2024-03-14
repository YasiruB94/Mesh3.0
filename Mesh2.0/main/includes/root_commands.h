#ifdef ROOT
#ifndef ROOT_COMMANDS_H
#define ROOT_COMMANDS_H

#include "root_utilities.h"

extern uint8_t PublishSensorData(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t PublishControlData(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t RestartRoot(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t TestRoot(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t SendGroupCmd(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t UpdateRootFW(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t UpdateNodeFW(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t CreateGroup(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t DeleteGroup(uint32_t uwValue, char *cptrString, uint8_t *ubyptrNodeMacs);
extern uint8_t TestGroup(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);
extern uint8_t AddOrgInfo(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);
extern uint8_t MessageToAWS(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);
extern uint8_t PublishNodeControlSuccess(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);
extern uint8_t PublishNodeControlFail(uint32_t uwValue, char *cptrString, uint8_t *ubyptrMacs);

enum enumRootCmndKey
{
    /************ ROOT NODE COMMANDS ************/
    enumRootCmndKey_RestartRoot = 50,
    enumRootCmndKey_TestRoot = 51,
    enumRootCmndKey_UpdateRootFW = 52, //Test this
    enumRootCmndKey_TestGroup = 53,
    enumRootCmndKey_SendGroupCmd = 54,
    enumRootCmndKey_AddOrgInfo = 55,
    enumRootCmndKey_PublishSensorData = 58,
    enumRootCmndKey_PublishControlData = 59,
    enumRootCmndKey_MessageToAWS = 60,
    enumRootCmndKey_UpdateNodeFW = 61, //test this
    enumRootCmndKey_CreateGroup = 62,
    enumRootCmndKey_DeleteGroup = 63,
    enumRootCmndKey_PublishNodeControlSuccess = 64,
    enumRootCmndKey_PublishNodeControlFail = 65,
    /************ TOTAL NUMBER OF COMMANDS ************/
    enumRootCmndKey_TotalNumOfCommands = 14
};

#endif
#endif