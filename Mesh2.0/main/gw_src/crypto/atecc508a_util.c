/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_util.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for utility events
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#define LOG_TAG "atecc508a_util"

/**
 * @brief time delay
 * @param delay
 */
void atecc508a_delay(size_t delay)
{
    vTaskDelay(delay / portTICK_PERIOD_MS);
}

/**
 * @brief Check CRC of the responses
 * @param response
 * @param length
 * @return esp_err_t
 */
esp_err_t atecc508a_check_crc(uint8_t *response, size_t length)
{
    uint16_t response_crc = response[length - 2] | (response[length - 1] << 8);
    uint16_t calculated_crc = atecc508a_crc(response, length - 2);

    return (response_crc == calculated_crc) ? ESP_OK : ESP_ERR_INVALID_CRC;
}
#endif