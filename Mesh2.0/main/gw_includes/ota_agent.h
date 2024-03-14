/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: ota_agent.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: link between main GW code and the ota_agent_core.c
 * includes code for IPNODE, GATEWAY_ETH, GATEWAY_SIM7080
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifdef IPNODE
#ifndef OTA_AGENT_H
#define OTA_AGENT_H
#include "gw_includes/ota_agent_core.h"

esp_err_t NODE_Mainboard_OTA_Begin();
esp_err_t NODE_Mainboard_OTA_Update_Data_Length_To_Be_Expected(size_t data, CNGW_Firmware_Binary_Type target_MCU, uint8_t major_version, uint8_t minor_version, uint8_t ci_version, uint8_t branch_id);
esp_err_t NODE_Mainboard_OTA_Process_Data(uint8_t *data, size_t data_len);
esp_err_t NODE_Mainboard_OTA_Write_Data(const uint8_t *data, size_t size);
bool NODE_do_Mainboard_OTA();

#endif
#endif

#ifdef ROOT
#ifndef OTA_AGENT_H
#define OTA_AGENT_H
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "math.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include <esp_partition.h>
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"
#include "esp_flash_partitions.h"

#ifdef GATEWAY_ETH
#include "gw_includes/ota_agent_core.h"
esp_err_t ETHROOT_Mainboard_OTA_Update_Data_Length_To_Be_Expected(size_t data, CNGW_Firmware_Binary_Type target_MCU, uint8_t major_version, uint8_t minor_version, uint8_t ci_version, uint8_t branch_id);
bool ETHROOT_do_Mainboard_OTA();
#endif

esp_err_t ota_begin();
esp_err_t ota_write_data(const uint8_t  *data, size_t size);
esp_err_t ota_end();
esp_err_t update_ota_data_length_to_be_expected(size_t data);
esp_err_t process_ota_data(uint8_t *data, size_t data_len);
uint32_t get_ota_start_address();

#endif
#endif

//if the GW is acting ONLY as a SIM7080 GW
#ifndef IPNODE
#ifdef GATEWAY_SIM7080
#ifndef OTA_AGENT_H
#define OTA_AGENT_H

#include "gw_includes/ota_agent_core.h"

esp_err_t SIM7080_Mainboard_OTA_Update_Data_Length_To_Be_Expected(size_t data, CNGW_Firmware_Binary_Type target_MCU, uint8_t major_version, uint8_t minor_version, uint8_t ci_version, uint8_t branch_id);
bool SIM7080_do_Mainboard_OTA();
esp_err_t update_ota_data_length_to_be_expected(size_t data);
esp_err_t ota_begin();
esp_err_t process_ota_data(uint8_t *data, size_t data_len);
esp_err_t ota_write_data(const uint8_t *data, size_t size);

#endif
#endif
#endif