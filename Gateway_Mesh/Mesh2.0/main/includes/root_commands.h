#ifdef ROOT
#ifndef ROOT_COMMANDS_H
#define ROOT_COMMANDS_H

#include "utilities.h"

typedef struct
{
    uint32_t uwValue;
    char *cptrString;
    uint8_t *ubyptrMacs;
    char *usrID;
    uint8_t ubyCommand;
} RootStruct_t;

extern uint8_t PublishSensorData(RootStruct_t *structRootReceived);
extern uint8_t PublishRegisterDevice(RootStruct_t *structRootReceived);
extern uint8_t RestartRoot(RootStruct_t *structRootReceived);
extern uint8_t TestRoot(RootStruct_t *structRootReceived);
extern uint8_t SendGroupCmd(RootStruct_t *structRootReceived);
extern uint8_t UpdateRootFW(RootStruct_t *structRootReceived);
extern uint8_t UpdateFW(RootStruct_t *structRootReceived);
extern uint8_t UpdateCenceFW(RootStruct_t *structRootReceived);
extern uint8_t CreateGroup(RootStruct_t *structRootReceived);
extern uint8_t DeleteGroup(RootStruct_t *structRootReceived);
extern uint8_t TestGroup(RootStruct_t *structRootReceived);
extern uint8_t AddOrgInfo(RootStruct_t *structRootReceived);
extern uint8_t MessageToAWS(RootStruct_t *structRootReceived);
extern uint8_t PublishNodeControlSuccess(RootStruct_t *structRootReceived);
extern uint8_t PublishNodeControlFail(RootStruct_t *structRootReceived);

enum enumRootCmndKey
{
    /************ ROOT NODE COMMANDS ************/
    enumRootCmndKey_RestartRoot                 = 50,
    enumRootCmndKey_TestRoot                    = 51,
    enumRootCmndKey_UpdateRootFW                = 52, // Test this
    enumRootCmndKey_TestGroup                   = 53,
    enumRootCmndKey_SendGroupCmd                = 54,
    enumRootCmndKey_AddOrgInfo                  = 55,
    enumRootCmndKey_PublishSensorData           = 58,
    enumRootCmndKey_PublishRegisterDevice       = 59,
    enumRootCmndKey_MessageToAWS                = 60,
    enumRootCmndKey_UpdateNodeFW                = 61, // test this
    enumRootCmndKey_CreateGroup                 = 62,
    enumRootCmndKey_DeleteGroup                 = 63,
    enumRootCmndKey_PublishNodeControlSuccess   = 64,
    enumRootCmndKey_PublishNodeControlFail      = 65,
    enumRootCmndKey_UpdateSensorFW              = 66,
    enumRootCmndKey_UpdateCenseFW               = 67,
    /************ TOTAL NUMBER OF COMMANDS ************/
    enumRootCmndKey_TotalNumOfCommands = 16
};

#endif
#endif