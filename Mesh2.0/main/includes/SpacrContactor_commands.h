#ifdef IPNODE
#ifndef SC_COMMANDS_H
#define SC_COMMANDS_H
#include "utilities.h"
#include "node_commands.h"
#include "node_utilities.h"

//relays
#define relay1 21
#define relay2 22
#define relay3 26
#define relay4 25
#define relayMasks ((1ULL << relay1) | (1ULL << relay2) | (1ULL << relay3) | (1ULL << relay4))

#define dRead1 19
#define dRead2 27
#define dMasks ((1ULL << dRead1) | (1ULL << dRead2))

//ADC GPIO Pins
#define adc1 35
#define adc2 34
#define adc3 39
#define adc0 36

void _setRelays(uint8_t *positions, uint8_t length);
void _loadRelayPositions();
void _initGpios(uint64_t mask, gpio_mode_t mode);
extern void SC_Init();
extern bool SC_SetRelays(NodeStruct_t *structNodeReceived);
extern bool SC_SelectADCs(NodeStruct_t *structNodeReceived);
extern bool SC_SetADCPeriod(NodeStruct_t *structNodeReceived);
extern bool SC_EnableDisableDigiReads(NodeStruct_t *structNodeReceived);

#endif
#endif