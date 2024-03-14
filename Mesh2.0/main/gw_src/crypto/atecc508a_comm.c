/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_comm.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for building communication with 
 * the crypto chip
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#define LOG_TAG "atecc508a_comm"

esp_err_t atecc508a_send_command(uint8_t command, uint8_t param1, uint16_t param2, uint8_t *data, size_t length)
{
    // 1. calculate header and length. It expects to see: word address, count, command opcode, param1, param2, data (optional), CRC[0], CRC[1]
    uint8_t total_transmission_length = 1 + 1 + 1 + sizeof(param1) + sizeof(param2) + length + sizeof(uint16_t);
    uint8_t header[] = {
        0x03,
        total_transmission_length - 1,
        command,
        param1,
        (param2 & 0xFF),
        (param2 >> 8) & 0xFF};

    // 2. calculate packet CRC
    atecc508a_crc_ctx_t ctx;
    uint16_t crc = 0;
    atecc508a_crc_begin(&ctx);
    atecc508a_crc_update(&ctx, header + 1, sizeof(header) - 1);
    atecc508a_crc_update(&ctx, data, length);
    atecc508a_crc_end(&ctx, &crc);

    esp_err_t result = ESP_FAIL;
    // 3. wake up the device
    result = atecc508a_wake_up();
    /*
    if (result != ESP_OK)
    {   ESP_LOGI(LOG_TAG, "TEST 01: %s", esp_err_to_name(result));
       // return result;
    }
    */

    // 4. send the data. initialize i2c transaction
    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        // address
        i2c_master_write_byte(cmd, (ATECC508A_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        // header
        i2c_master_write(cmd, header, sizeof(header), ACK_CHECK_EN);
        // data
        if (data != NULL)
        {
            i2c_master_write(cmd, data, length, ACK_CHECK_EN);
        }
        // crc
        i2c_master_write(cmd, (uint8_t *)&crc, sizeof(uint16_t), ACK_CHECK_EN);
        i2c_master_stop(cmd);
        // begin the reansaction
        result = i2c_master_cmd_begin(I2C_PORT, cmd, ATMEL_RESPONSE_TIMEOUT_GENERIC / portTICK_RATE_MS);
        // delete the i2c initialization to free resources
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }

    // return the result of the transaction
    return result;
}

esp_err_t atecc508a_receive(uint8_t *buffer, size_t length)
{
    size_t received = 0;
    size_t left = length;
    uint8_t retries = 0;

    while (left > 0)
    {
        uint8_t requestLength = left > 32 ? 32 : left;
        esp_err_t ret = ESP_FAIL;
        if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (ATECC508A_ADDR << 1) | I2C_MASTER_READ, ACK_CHECK_DIS);

            if (requestLength > 1)
            {
                i2c_master_read(cmd, buffer + received, requestLength - 1, ACK_CHECK_DIS);
            }
            i2c_master_read_byte(cmd, buffer + received + requestLength - 1, ACK_CHECK_EN);
            i2c_master_stop(cmd);
            ret = i2c_master_cmd_begin(I2C_PORT, cmd, ATMEL_RESPONSE_TIMEOUT_GENERIC / portTICK_RATE_MS);
            i2c_cmd_link_delete(cmd);

            // Release the semaphore when done
            xSemaphoreGive(Semaphore_I2C_Resource);
        }

        if (ret == ESP_OK)
        {
            received += requestLength;
            left -= requestLength;
        }

        if (retries++ >= 20)
        {
            return ESP_ERR_TIMEOUT;
        }
    }

    ESP_LOG_BUFFER_HEX_LEVEL(LOG_TAG, buffer, length, ESP_LOG_DEBUG);

    return ESP_OK;
}

esp_err_t atecc508a_read(uint8_t zone, uint16_t address, uint8_t *buffer, uint8_t length)
{
    // adjust zone as needed for whether it's 4 or 32 bytes length read
    // bit 7 of zone needs to be set correctly
    // (0 = 4 Bytes are read)
    // (1 = 32 Bytes are read)
    if (length == 32)
    {
        zone |= 0b10000000; // set bit 7
    }
    else if (length == 4)
    {
        zone &= ~0b10000000; // clear bit 7
    }
    else
    {
        return ESP_ERR_INVALID_ARG; // invalid length, abort.
    }

    atecc508a_send_command(ATCA_READ, zone, address, NULL, 0);

    atecc508a_delay(1);

    uint8_t tmp_buf[35];

    atecc508a_receive(tmp_buf, sizeof(tmp_buf));

    atecc508a_idle();

    atecc508a_check_crc(tmp_buf, sizeof(tmp_buf));

    // copy response
    memcpy(buffer, tmp_buf + 1, length);

    return ESP_OK;
}

esp_err_t atecc508a_write(uint8_t zone, uint16_t address, uint8_t *buffer, uint8_t length)
{
    // adjust zone as needed for whether it's 4 or 32 bytes length read
    // bit 7 of zone needs to be set correctly
    // (0 = 4 Bytes are read)
    // (1 = 32 Bytes are read)
    if (length == 32)
    {
        zone |= 0b10000000; // set bit 7
    }
    else if (length == 4)
    {
        zone &= ~0b10000000; // clear bit 7
    }
    else
    {
        return ESP_ERR_INVALID_ARG; // invalid length, abort.
    }

    atecc508a_send_command(ATCA_WRITE, zone, address, buffer, length);

    atecc508a_delay(26);

    uint8_t tmp_buf[4];

    atecc508a_receive(tmp_buf, sizeof(tmp_buf));

    atecc508a_idle();

    atecc508a_check_crc(tmp_buf, sizeof(tmp_buf));

    if (tmp_buf[1] != 0x00)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}
#endif
