/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: cence_sha256.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used to provision a crypto chip, all SHA256 related
 * functions
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CENSESHA256_H
#define CENSESHA256_H
#include "includes/SpacrGateway_commands.h"

struct SHA256_CTX_t{
	uint8_t data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[8];
};

typedef struct SHA256_CTX_t * SHA256_CTX_p;

void CENSE_Sha256_Start(SHA256_CTX_p ctx);
void CENSE_Sha256_Update(SHA256_CTX_p ctx, const uint8_t * data, uint32_t len);
void CENSE_Sha256_Update_Zeros(SHA256_CTX_p ctx, uint32_t length);
void CENSE_Sha256_Final(SHA256_CTX_p ctx, uint8_t * hash);

#endif
#endif