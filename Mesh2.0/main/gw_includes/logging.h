/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: logging.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in handling logging messages from mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef LOGGING_H
#define LOGGING_H
#include "includes/SpacrGateway_commands.h"
#include "handshake.h"


void Handle_Log_Message(const uint8_t *recvbuf);
void Handle_Error_Message(const uint8_t *recvbuf);

#endif
#endif