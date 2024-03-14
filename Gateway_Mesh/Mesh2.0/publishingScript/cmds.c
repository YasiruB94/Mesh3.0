{
    /************ ROOT NODE COMMANDS ************/
    enumRootCmndKey_RestartRoot = 50,
    enumRootCmndKey_TestRoot = 51,
    enumRootCmndKey_UpdateRootFW = 52, // Test this
    enumRootCmndKey_TestGroup = 53,
    enumRootCmndKey_SendGroupCmd = 54,
    enumRootCmndKey_AddOrgInfo = 55,
    enumRootCmndKey_PublishSensorData = 58,
    enumRootCmndKey_MessageToAWS = 60,
    enumRootCmndKey_UpdateNodeFW = 61, // test this
    enumRootCmndKey_CreateGroup = 62,
    enumRootCmndKey_DeleteGroup = 63,
    enumRootCmndKey_PublishNodeControlSuccess = 64,
    enumRootCmndKey_PublishNodeControlFail = 65,
    enumRootCmndKey_UpdateSensorFW = 66,
    /************ TOTAL NUMBER OF COMMANDS ************/
    enumRootCmndKey_TotalNumOfCommands = 15};
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
    enumNodeCmndKey_SendFirmwareVersion = 13,
    enumNodeCmndKey_GetNodeOutput = 14,
    enumNodeCmndKey_GetParentRSSI = 15,
    enumNodeCmndKey_GetNodeChildren = 16,
    enumNodeCmndKey_SetAutoSensorPairing = 17,
    enumNodeCmndKey_SetSensorMeshID = 18,
    enumNodeCmndKey_SendSensorFirmware = 19,
    enumNodeCmndKey_ClearSensorCommand = 20,
    enumNodeCmndKey_ClearNVS = 21,
    enumNodeCmndKey_TotalNumOfCommands = 23

};