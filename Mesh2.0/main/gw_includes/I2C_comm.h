/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: I2C_comm.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: All I2C communication functions used in Crypto Chip and LED driver control
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef I2C_COMM_H
#define I2C_COMM_H
#include "includes/SpacrGateway_commands.h"
#ifdef GATEWAY_ETH
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "esp_log.h"
#endif

#ifdef USING_OLD_GW
#define I2C_MASTER_SDA_IO 	21
#define I2C_MASTER_SCL_IO 	22
#define I2C_MASTER_FREQ_HZ 	100000

#else
#define I2C_MASTER_SDA_IO 	32
#define I2C_MASTER_SCL_IO 	33
#define I2C_MASTER_FREQ_HZ 	100000

#endif

SemaphoreHandle_t Semaphore_I2C_Resource;
i2c_port_t I2C_PORT;
esp_err_t init_I2C();
esp_err_t delete_I2C();

#endif
#endif