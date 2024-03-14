#ifdef ROOT

#ifndef _ROOTOPERATIONS_H_
#define _ROOTOPERATIONS_H_

#include "root_utilities.h"
#include "utilities.h"

extern QueueHandle_t nodeCommandQueue;
extern QueueHandle_t rootCommandQueue;
extern QueueHandle_t AWSPublishQueue;
extern void RootOperations_ProcessNodeCommands();
extern void RootOperations_RootGroupReadTask(void *arg);
extern void RootOperations_ProcessRootCommands();
extern void RootOperations_RootReadTask(void *arg);

#endif
#endif