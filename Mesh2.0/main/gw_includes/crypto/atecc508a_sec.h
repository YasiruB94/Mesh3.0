/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_sec.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for all security commands
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_SEC_H
#define ATECC508A_SEC_H

#define LOCK_MODE_ZONE_CONFIG       0b10000000
#define LOCK_MODE_ZONE_DATA_AND_OTP 0b10000001
#define LOCK_MODE_SLOT0             0b10000010

esp_err_t atecc508a_lock(uint8_t zone);
esp_err_t atecc508a_create_key_pair();

#endif
#endif