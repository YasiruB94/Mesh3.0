/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: control.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending control commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CONTROL_H
#define CONTROL_H
#include "includes/SpacrGateway_commands.h"
#include "handshake.h"
#include "cngw_structs/cngw_control.h"

esp_err_t send_control_message(CNGW_Control_Command cmd, CNGW_Log_Level lvl, CNGW_Source_MCU MCU);

#endif
#endif