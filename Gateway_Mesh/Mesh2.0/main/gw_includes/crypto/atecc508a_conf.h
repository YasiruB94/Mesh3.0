/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_conf.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for setting up and referring to 
 * configuration zones
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_CONF_H
#define ATECC508A_CONF_H

esp_err_t atecc508a_read_config_zone();
esp_err_t atecc508a_is_configured(uint8_t *is_configured);
esp_err_t atecc508a_configure();
esp_err_t atecc508a_write_config();

#endif
#endif