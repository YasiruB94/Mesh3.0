/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: direct_control.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending direct control commands to mainboard
 * NOTE: New addition to the cence FW > 2.5.3.0. used in sending new information like
 * updating the current for drivers, etc
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef DIRECT_CONTROL_H
#define DIRECT_CONTROL_H
#include "includes/SpacrGateway_commands.h"
#include "handshake.h"
#include "cngw_structs/cngw_direct_control.h"

esp_err_t send_direct_control_message(CNGW_Direct_Control_Command cmd, CNGW_Source_MCU target_MCU, uint8_t uint8_value, uint16_t uint16_value, uint32_t int_value);

#endif
#endif