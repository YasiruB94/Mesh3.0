/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: ota_agent.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: link between main GW code and the ota_agent_core.c
 * includes code for GATEWAY_ETHERNET, GATEWAY_SIM7080
 * 
 ******************************************************************************
 * The code for IP_NODE is removed since the GW is only of type ROOT now.
 * To get the code intended for GATEWAY_IPNODE, refer the branch CF-70-ETHERNET
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef OTA_AGENT_H
#define OTA_AGENT_H

#include "gw_includes/ota_agent_core.h"
extern bool restart_ROOT_after_OTA;

esp_err_t GW_ROOT_Mainboard_OTA_Update_Variables(size_t data, CNGW_Firmware_Binary_Type target_MCU, uint8_t major_version, uint8_t minor_version, uint8_t ci_version, uint8_t branch_id);
bool GW_ROOT_do_Mainboard_OTA();
esp_err_t GW_ROOT_find_OTA_partition();
esp_err_t GW_ROOT_write_OTA_packet(const uint8_t  *data, size_t size);
esp_err_t GW_ROOT_OTA_end();
esp_err_t GW_ROOT_prepare_memory_to_store_OTA_data_temporarily(size_t data);
esp_err_t GW_ROOT_process_OTA_packet(uint8_t *data, size_t data_len);
uint32_t GW_ROOT_get_OTA_start_address();
esp_err_t GW_ROOT_Check_ESP32_FW_version(char * URL);

#ifdef GATEWAY_ETHERNET
#include "includes/root_utilities.h"
bool GW_ROOT_UpgradeRootESP32Firmware(MeshStruct_t *structRootWrite);
#endif

#endif
#endif
