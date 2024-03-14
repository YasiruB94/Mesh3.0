/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: action.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in sending action commands to mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef ACTION_H
#define ACTION_H
#include "includes/SpacrGateway_commands.h"
#include "handshake.h"

esp_err_t Send_Action_Msg_On_Off(uint16_t address, CNGW_Action_Command cmd, uint16_t level);
esp_err_t Send_Action_Msg_broadcast(CNGW_Action_Command cmd, uint16_t level,  CNGW_Fade_Unit fade_unit, uint16_t fade_time);
esp_err_t Send_Action_Msg_fade(uint16_t address, CNGW_Action_Command cmd, uint16_t level, CNGW_Fade_Unit fade_unit, uint16_t fade_time);

#endif
#endif