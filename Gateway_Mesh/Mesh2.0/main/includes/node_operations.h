#ifdef IPNODE
#include "node_commands.h"
#include "esp_now_utilities.h"
#include "sensor_commands.h"

extern void NodeOperations_NodeReadTask(void *arg);
extern void NodeOperations_CommandExecutionTask(void *arg);
extern void NodeOperations_RootSendTask(void *arg);
extern void NodeOperations_ProcessEspNowCommands();
void NodeOperations_ReadFromSensorTask(void *arg);
#endif