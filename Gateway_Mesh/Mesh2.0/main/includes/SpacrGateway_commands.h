/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: SpacrGateway_commands.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: main C file for the GW. Handles the GW initialization. 
 * This file includes functions to handle commands and messages coming from ROOT,
 * prepare the response message to AWS, and GW initialization.
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef GW_COMMANDS_H
#define GW_COMMANDS_H

#include "driver/spi_slave.h"
#include "esp_spi_flash.h"
#include "utilities.h"
#include "node_commands.h"
#include "node_utilities.h"
#include "../gw_includes/cngw_structs/cngw_action.h"
#include "../gw_includes/cngw_structs/cngw_handshake.h"
#include "../gw_includes/cngw_structs/cngw_miscellaneous.h"
#include "../gw_includes/ccp_util.h"
#include "../gw_includes/gpio.h"
#include "../gw_includes/status_led.h"
#include "../gw_includes/SPI_comm.h"
#include "../gw_includes/handshake.h"
#include "../gw_includes/action.h"
#include "../gw_includes/query.h"
#include "../gw_includes/control.h"
#include "../gw_includes/direct_control.h"
#include "../gw_includes/channel.h"

#include "../gw_includes/ota_agent_core.h"
#include "../gw_includes/handle_commands.h"
#include "../gw_includes/I2C_comm.h"
#include "../gw_includes/machine_state.h"
#include "../gw_includes/configuration.h"
#include "../gw_includes/crypto/atecc508a.h"
#include "../gw_includes/crypto/crypto.h"
#include "../gw_includes/crypto/atecc508a_comm.h"
#include "../gw_includes/crypto/atecc508a_conf.h"
#include "../gw_includes/crypto/atecc508a_crc.h"
#include "../gw_includes/crypto/atecc508a_pwr.h"
#include "../gw_includes/crypto/atecc508a_sec.h"
#include "../gw_includes/crypto/atecc508a_util.h"
#include "../gw_includes/crypto/cense_sha256.h"
#include "../gw_includes/crypto/cence_crypto_chip_provision.h"
#include "../gw_includes/cence_crc.h"

#ifndef GATEWAY_IPNODE
typedef struct
{
    uint8_t ubyCommand;
    double dValue;
    uint8_t *arrValues;
    uint8_t arrValueSize;
    char *cptrString;
    bool group;
    char *usrID;
} NodeStruct_t;
#endif

#include "../gw_includes/secondaryUtilities.h"
#include "../gw_includes/SIM7080.h"
#include "../gw_includes/ota_agent.h"

extern CNGW_CN_Board_Info_t cn_board_info;
extern struct CN_Switch_Configuration_t wired_sw_configuration[WIRED_SWITCH_COUNT];
extern struct CN_Switch_Configuration_t wireless_sw_configuration[WIRELESS_SWITCH_COUNT];
extern CN_Sensor_Configuration_t sensor_configuration[MAX_SENSORS];
extern CN_DRV_Slot_t driver_slots[CABINET__DRIVER_SLOT_COUNT];

extern void Send_GW_message_to_AWS(uint16_t ubyCommand, uint32_t uwValue, char *cptrString);
extern void Initialize_Gateway();
extern bool GW_Process_Action_Command(NodeStruct_t *structNodeReceived);
extern bool GW_Process_AT_Command(NodeStruct_t *structNodeReceived);
extern bool GW_Process_Action_Command_As_ByteArray(NodeStruct_t *structNodeReceived);
extern bool GW_Process_Query_Command(NodeStruct_t *structNodeReceived);
extern bool GW_Process_OTA_Command_Begin(NodeStruct_t *structNodeReceived);
extern bool GW_Process_OTA_Command_Data(const char *value);
extern bool GW_Process_OTA_Command_End(NodeStruct_t *structNodeReceived);
bool Send_Response_To_AWS(CNGW_AWS_Response_t *data, uint8_t size);

#endif
#endif