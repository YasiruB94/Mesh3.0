/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_util.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for utility events
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_UTIL_H
#define ATECC508A_UTIL_H

void atecc508a_delay(size_t delay);
esp_err_t atecc508a_check_crc(uint8_t *response, size_t length);

#endif
#endif