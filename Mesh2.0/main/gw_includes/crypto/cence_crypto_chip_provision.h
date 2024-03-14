/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: cence_crypto_chip_provision.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used to provision a crypto chip to the requirements needed
 * to achieve proper handshake with mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CRYPTO_CHIP_PROVISION_H
#define CRYPTO_CHIP_PROVISION_H
#include "includes/SpacrGateway_commands.h"

esp_err_t ATMEL508_get_random_number(uint8_t *random_num);
esp_err_t ATMEL508_check_configuration_zone_state();
esp_err_t Verify_Keys(void);
esp_err_t ATMEL508_personalize_configuration_zone();
esp_err_t ATMEL508_personalize_data_zone();
esp_err_t ATMEL508_provision_chip();

#endif
#endif