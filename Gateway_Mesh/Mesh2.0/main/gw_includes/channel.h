/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: channel.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending channel commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CHANNEL_H
#define CHANNEL_H
#include "includes/SpacrGateway_commands.h"
#include "handshake.h"
#include "cngw_structs/cngw_config.h"

esp_err_t send_channel_message(const CNGW_Config_Status msg_status);

#endif
#endif