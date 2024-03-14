/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: query.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in handling query messages from mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef QUERY_H
#define QUERY_H
#include "includes/SpacrGateway_commands.h"
#include "handshake.h"
#include "cngw_structs/cngw_final_frame.h"
#include "cngw_structs/cngw_query.h"

esp_err_t query_all_channel_info();

#endif
#endif