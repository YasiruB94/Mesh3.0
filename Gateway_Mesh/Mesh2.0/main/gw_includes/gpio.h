/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: gpio.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in configuraing GPIO pins of the ESP32 according to
 * the requirement
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef GPIO_H
#define GPIO_H
#include "includes/SpacrGateway_commands.h"
#include "freertos/semphr.h"

SemaphoreHandle_t PowerFailSemaphore;

#ifdef USING_OLD_GW
#define BDATA_READY 27
#define GW_EX1      33
#define GW_ON       26
#define POWER_DETECT_PIN 34

#else
#define BDATA_READY 4
#define GW_EX1      2
#define GW_ON       5
#define POWER_DETECT_PIN 34

#endif


esp_err_t init_output_GPIO();
esp_err_t init_input_power_GPIO();
esp_err_t Reset_GW();
esp_err_t Signal_GW_Of_Sending_Data();
esp_err_t Backward_Data_Sequence();
esp_err_t Backward_Data_Ready_Sequence();
esp_err_t Tx_Complete_Sequence();
#ifdef GATEWAY_SIM7080
esp_err_t init_SIM7080_GPIO();
esp_err_t Reset_SIM7080();
#endif

#endif
#endif