/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: secondaryUtilities.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: codes involved in GW processing the incoming messages just as it will if it is a GATEWAY_IPNODE.
 * The following code is intelded ONLY for GATEWAY_SIM7080 or GATEWAY_ETHERNET modes
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_SIM7080) || defined(GATEWAY_ETHERNET)
#ifndef SECONDARYUTILITIES_H
#define SECONDARYUTILITIES_H

#include "includes/SpacrGateway_commands.h"
#ifdef GATEWAY_ETHERNET
#include "includes/root_utilities.h"
#endif

#ifdef GATEWAY_SIM7080
void SIM7080_AWS_Tx_task(void *pvParameters);
void SIM7080_CommandExecutionTask(void *arg);
#endif
bool SecondaryUtilities_ValidateAndExecuteCommand(NodeStruct_t *structNodeReceived);
void SecondaryUtilities_PrepareJSONAndSendToAWS(uint16_t ubyCommand, uint32_t uwValue, char *cptrString);

#endif
#endif