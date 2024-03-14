/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: configuration.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending configuration commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include "includes/SpacrGateway_commands.h"
extern TaskHandle_t ConfigTask;

void Handle_Channel_Status_Message(const uint8_t *recvbuf);
void Handle_Configuration_Information_Message(const uint8_t *recvbuf);
void Request_Configuration_information();
void configuration_request_loop(void *pvParameters);

#endif
#endif