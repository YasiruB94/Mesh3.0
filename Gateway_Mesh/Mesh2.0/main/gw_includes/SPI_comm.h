/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: SPI_comm.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in SPI communication with CENCE mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef SPI_COMM_H
#define SPI_COMM_H
#include "includes/SpacrGateway_commands.h"
#include "gpio.h"
#include "handshake.h"
#include "cngw_structs/cngw_handshake.h"

#define GPIO_MOSI       13
#define GPIO_MISO       12
#define GPIO_SCLK       14
#define GPIO_CS         15
#define BUFFER          134
#define QUEUE_LENGTH    200

extern QueueHandle_t    GW_response_queue; 
extern QueueHandle_t    CN_message_queue;
extern bool             cn_message_queue_error;

esp_err_t init_GW_SPI_communication();
void post_setup_cb(spi_slave_transaction_t *trans);
void post_trans_cb(spi_slave_transaction_t *trans);
void GW_Trancieve_Data(void *pvParameters);
esp_err_t is_all_zeros(const uint8_t *array, size_t size);
esp_err_t consume_GW_message(uint8_t *message);
void GW_process_received_data(void *pvParameters);
size_t Parse_1_Frame_Partial(uint8_t *packet, size_t dataSize, size_t startIndex);
esp_err_t free_SPI(CNGW_Firmware_Binary_Type target_MCU_int);

#endif
#endif