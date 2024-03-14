#ifdef IPNODE
#ifndef SIO_COMMANDS_H
#define SIO_COMMANDS_H
#include "utilities.h"
#include "node_commands.h"
#include "node_utilities.h"

#define gndCutOff 25
#define RELAY1 4
#define D1 27
#define D2 26
#define D3 19 //0
#define D4 18 //1

#define adc1 35
#define adc2 34
#define adc3 39
#define adc4 36

#define VREF_IO 1100

#define outputMasksIO ((1ULL << D1) | (1ULL << D2) | (1ULL << D3) | (1ULL << D4) | (1ULL << RELAY1) | (1ULL << gndCutOff))
#define inputMasksIO (1ULL << adc4)

void _initSIO();
void _loadADCAndPeriod();
void _loadEnableDisableSub();
void _loadVoltageAndCutoff();
void _loadRelayPosition();
extern void SIO_Init();
extern bool SIO_SetVoltage(NodeStruct_t *structNodeReceived);
extern bool SIO_SelectADCs(NodeStruct_t *structNodeReceived);
extern bool SIO_SetADCPeriod(NodeStruct_t *structNodeReceived);
extern bool SIO_EnableDisableSub(NodeStruct_t *structNodeReceived);
extern bool SIO_SetRelay(NodeStruct_t *structNodeReceived);

#endif
#endif