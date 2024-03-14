/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: handle_commands.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in decoding the recieving commands/ messages from
 * mainboard and passing the decoded infomation to the approptiate functions
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef HANDLE_COMMANDS_H
#define HANDLE_COMMANDS_H

#include "includes/SpacrGateway_commands.h"
#include "handshake.h"

uint8_t Is_Header_Valid(const CNGW_Message_Header_t *const header);
size_t Parse_1_Frame(uint8_t *packet, size_t dataSize);
void printBits(uint32_t num);

#endif
#endif