/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_crc.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for all CRC related events
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_CRC_H
#define ATECC508A_CRC_H
#include "at508_structs.h"

void atecc508a_crc_begin(atecc508a_crc_ctx_t *ctx);
void atecc508a_crc_update(atecc508a_crc_ctx_t *ctx, uint8_t *data, size_t length);
void atecc508a_crc_end(atecc508a_crc_ctx_t *ctx, uint16_t *crc);
uint16_t atecc508a_crc(uint8_t *data, size_t length);

#endif
#endif