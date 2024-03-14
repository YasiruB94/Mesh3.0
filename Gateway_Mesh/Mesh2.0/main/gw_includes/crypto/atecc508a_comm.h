/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_comm.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for building communication with 
 * the crypto chip
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_COMM_H
#define ATECC508A_COMM_H

esp_err_t atecc508a_send_command(uint8_t command, uint8_t param1, uint16_t param2, uint8_t *data, size_t length);
esp_err_t atecc508a_receive(uint8_t *buffer, size_t length);
esp_err_t atecc508a_read(uint8_t zone, uint16_t address, uint8_t *buffer, uint8_t length);
esp_err_t atecc508a_write(uint8_t zone, uint16_t address, uint8_t *buffer, uint8_t length);

#endif
#endif