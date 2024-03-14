#ifdef IPNODE
#ifndef SP_COMMANDS_H
#define SP_COMMANDS_H
#include "utilities.h"
#include "node_commands.h"
#include "node_utilities.h"
#include "MCP4551.h"
#include "driver/ledc.h"

#define MIN_POT_VALUE 20
#define MAX_POT_VALUE 255

#define MIN_DUTY_VALUE 0
#define MAX_DUTY_VALUE 1023

#define LEDC_FREQ 40000
#define LEDC_FADE_TIME_MS 250 //changed to shorter PWm delay

void _initLedDriver();
void _setPwm(uint8_t value, uint8_t port);
void _loadVoltageAndPWMValues();
void _reportCurrentToRoot();
void _startCurrentSubmission();
void _initPowerNode();
void _loadCurrentSubTask();
extern void SP_Init();
extern bool SP_SetVoltage(NodeStruct_t *structNodeReceived);
extern bool SP_SetPWMPort0(NodeStruct_t *structNodeReceived);
extern bool SP_SetPWMPort1(NodeStruct_t *structNodeReceived);
extern bool SP_SetPWMBothPort(NodeStruct_t *structNodeReceived);
extern bool SP_EnableCurrentSubmission(NodeStruct_t *structNodeReceived);

#endif
#endif