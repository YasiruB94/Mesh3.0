/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_pwr.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for waking up and sleeping the
 * chip
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_PWR_H
#define ATECC508A_PWR_H

esp_err_t atecc508a_wake_up();
esp_err_t atecc508a_sleep();
esp_err_t atecc508a_idle();

#endif
#endif