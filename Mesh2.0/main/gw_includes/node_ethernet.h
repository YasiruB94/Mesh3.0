/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: node_ethernet.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: Legacy code for NODE acting as a ROOT. This code is not being used now
 ******************************************************************************
 *
 ******************************************************************************
 */
#ifdef IPNODE
#ifndef ETH_COMM_H
#define ETH_COMM_H

#include "includes/SpacrGateway_commands.h"

#include "eth_phy/phy_lan8720.h"
#include "esp_eth.h"
#include "rom/gpio.h"
#include "tcpip_adapter.h"
#include "driver/periph_ctrl.h"
#include "esp_event_loop.h"


#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define PHY_RESET_CHANNEL   11U
#define PIN0_ON_L           0x6 
#define PIN_MULTIPLYER      4 
#define PIN_SMI_MDC         23
#define PIN_SMI_MDIO        18
#define CONFIG_PHY_ADDRESS  1
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config


#define DEST_HOST "www.google.com"
#define DEST_PORT 80


esp_err_t Node_Ethernet_Init();
esp_err_t PCA9685_Set_Pin(uint8_t num, bool state);
void ping_task(void *pvParameters);

#endif
#endif