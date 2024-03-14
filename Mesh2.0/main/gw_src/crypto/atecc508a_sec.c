/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_sec.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for all security commands
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#define LOG_TAG "atecc508a_sec"

esp_err_t atecc508a_lock(uint8_t zone)
{
    ESP_ERROR_CHECK(atecc508a_send_command(ATCA_LOCK, zone, 0x0000, NULL, 0));

    atecc508a_delay(32);

    uint8_t resp[4];

    ESP_ERROR_CHECK(atecc508a_receive(resp, sizeof(resp)));

    atecc508a_idle();

    atecc508a_check_crc(resp, sizeof(resp));

    if (resp[1] != 0x00)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

esp_err_t atecc508a_create_key_pair()
{

    return ESP_OK;
}
#endif