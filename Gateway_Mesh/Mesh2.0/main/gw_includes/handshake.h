/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: handshake.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in achieving a proper handshake with the mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef HANDSHAKE_H
#define HANDSHAKE_H
#include "includes/SpacrGateway_commands.h"
#include "SPI_comm.h"

#define CNGW_SET_HEADER_DATA_SIZE(header, word_to_set) ((header)->data_size = word_to_set)

void analyze_CNGW_Handshake_CN1_t(const uint8_t *recvbuf, int size);
void analyze_CNGW_Handshake_CN2_t(const uint8_t *recvbuf, int size);

void prepare_CN_handshake_01_feedback(CNGW_Handshake_CN1_Frame_t *cn1);
void prepare_CN_handshake_02_feedback(CNGW_Handshake_CN2_Frame_t *cn2);
const CNGW_Firmware_Version_t *GWVer_Get_Firmware(void);
const CNGW_Firmware_Version_t *GWVer_Get_Bootloader_Firmware(void);

#endif
#endif