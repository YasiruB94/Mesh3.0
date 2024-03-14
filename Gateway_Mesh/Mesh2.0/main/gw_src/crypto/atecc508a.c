/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip to get a response and to generate
 * a SALT
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#include "mbedtls/sha256.h"
#define LOG_TAG "atecc508a"

esp_err_t atecc508a_response(uint8_t *response, uint8_t length, uint8_t command, uint8_t param1, uint16_t param2, uint8_t *data, size_t ota_agent_core_data_length, uint8_t delay_between_send_and_recv)
{
    uint8_t temp_buffer[length];
    // 1. send command to chip
    esp_err_t result = atecc508a_send_command(command, param1, param2, data, ota_agent_core_data_length);
    if (result != ESP_OK)
    {
        return result;
    }
    // 2. add a delay before trying to read the response from chip
    atecc508a_delay(delay_between_send_and_recv);
    // 3. receive data from chip
    result = atecc508a_receive(temp_buffer, sizeof(temp_buffer));
    if (result != ESP_OK)
    {
        return result;
    }
    // 4. set chip to idle mode
    result = atecc508a_idle();
    if (result != ESP_OK)
    {
        return result;
    }
    // 5. check the CRC of the received data
    result = atecc508a_check_crc(temp_buffer, sizeof(temp_buffer));
    if (result != ESP_OK)
    {
        return result;
    }
    // 6. copy the received data to the response buffer
    memcpy(response, temp_buffer, length);

    return result;
}


void ATMELPLA_Generate_Salt(uint8_t *salt)
{
    uint32_t rnd;

    for (uint32_t i = 0; i < 5; i++)
    {
        const uint32_t pos = i * 4;
        rnd = rand();

        salt[pos] = rnd & 0xFF;
        salt[pos + 1] = ((rnd >> 8) & 0xFF);
        salt[pos + 2] = ((rnd >> 16) & 0xFF);
        salt[pos + 3] = ((rnd >> 24) & 0xFF);
    }
}

#endif