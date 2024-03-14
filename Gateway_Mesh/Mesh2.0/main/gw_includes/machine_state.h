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

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#include "includes/SpacrGateway_commands.h"
#include "freertos/semphr.h"

#define MAX_HS_ATTEMPTS_ALLOWED             5
// Gateway version
#define GATEWAY_HARDWARE_VERSION_MAJOR      0
#define GATEWAY_HARDWARE_VERSION_MINOR      31
#define GATEWAY_HARDWARE_VERSION_CI         1
#define GATEWAY_FIRMWARE_VERSION_MAJOR      1
#define GATEWAY_FIRMWARE_VERSION_MINOR      1
#define GATEWAY_FIRMWARE_VERSION_CI         15
#ifdef GATEWAY_ETHERNET
#define GATEWAY_FIRMWARE_VERSION_BRANCH_ID  0 // branch Id for a ROOT+GATEWAY_ETHERNET is 0
#else
#define GATEWAY_FIRMWARE_VERSION_BRANCH_ID  1 // branch Id for a ROOT+GATEWAY_SIM7080  is 1
#endif

extern TaskHandle_t PowerFailureTask;
extern TimerHandle_t GW_Availability_Timer;
extern TimerHandle_t GW_Delayed_Restart_Timer;
extern bool power_failure_task_completed;
extern bool atleast_one_handshake_attempt_recieved;

void update_GW_version();
void check_for_GW_availability();
void check_for_GW_periodically(void *timer);
void error_handler(const char* context, esp_err_t reason);
void ESP_restart_sequence(void *timer);
void delayed_ESP_Restart(uint16_t delay_time);
void task_handle_power_failure(void *pvParameters);
void power_failure_handle_sequence();
void ESP_print_free_heap();
void ESP_mac_to_string(uint8_t mac[], char str[]);


#endif
#endif