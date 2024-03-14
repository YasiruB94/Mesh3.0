/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: ota_agent_core.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: Code involved in sending FW data to CENCE mainboard.
 * 
 * NOTE: If GATEWAY_ETH, no bundling occurs, and 20ms delay each 100 packets
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef OTA_AGENT_CORE_H
#define OTA_AGENT_CORE_H
#include "includes/SpacrGateway_commands.h"
#include "cngw_structs/cngw_ota.h"
#include "handshake.h"

extern bool                         ota_agent_core_OTA_in_progress;
extern uint32_t                     ota_agent_core_total_received_data_len;
extern uint32_t                     ota_agent_core_target_address;
extern uint32_t                     ota_agent_core_target_start_address;
extern uint32_t                     ota_agent_core_data_length;
extern CNGW_Firmware_Binary_Type    ota_agent_core_target_MCU;
extern uint8_t                      ota_agent_core_major_version;
extern uint8_t                      ota_agent_core_minor_version;
extern uint8_t                      ota_agent_core_CI_version;
extern uint8_t                      ota_agent_core_branch_ID;
extern bool                         ota_agent_core_OTA_FW_accepted_by_CN;
extern const esp_partition_t              *ota_agent_core_update_partition;

void OTA_FW_Success();
void OTA_Restart_Required();
void OTA_next_Frame(bool state_transfer);
void OTA_Init(const uint32_t session_timeout_ticks);
GW_STATUS OTA_Send_Binary(Binary_Data_Pkg_Info_t binary_file);

#endif
#endif