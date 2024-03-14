/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: machine_state.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in general ESP32 maintenance
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include "includes/SpacrGateway_commands.h"
#include "freertos/semphr.h"

extern TaskHandle_t PowerFailureTask;
extern TimerHandle_t GW_Availability_Timer;
extern TimerHandle_t GW_Delayed_Restart_Timer;
extern bool power_failure_task_completed;


void check_for_GW_availability();
void check_for_GW_periodically(void *timer);
void error_handler(const char* context, esp_err_t reason);
void ESP_restart_sequence(void *timer);
void delayed_ESP_Restart(uint16_t delay_time);
void task_handle_power_failure(void *pvParameters);
void power_failure_handle_sequence();


#endif
#endif