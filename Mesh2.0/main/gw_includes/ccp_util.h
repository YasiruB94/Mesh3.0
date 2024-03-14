/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: crypto.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in calculating the CRC 8 value for data validity
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CCP_UTIL_H
#define CCP_UTIL_H
#include "includes/SpacrGateway_commands.h"

#define SECOND_SINCE_1970		((uint32_t)(1522340061))
#define	MAJOR_VER				2
#define	MINOR_VER				5
#define CI_BLD_NUM				0
#define GIT_BRCH_ID				0


uint8_t CCP_UTIL_Get_Crc8(uint8_t crc, const uint8_t *buffer, uint16_t size);
void CCP_UTIL_Get_Msg_Header(CNGW_Message_Header_t *const header, CNGW_Header_Type command, const size_t data_size);
#endif
#endif