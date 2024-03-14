/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_pwr.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for waking up and sleeping the
 * chip
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#define LOG_TAG "atecc508a_pwr"

esp_err_t atecc508a_wake_up()
{
    esp_err_t ret = ESP_FAIL;
    // 1. set up to write to address "0x00"
    // This creates a "wake condition" where SDA is held low for at least tWLO
    // tWLO means "wake low duration" and must be at least 60 uSeconds (which is acheived by writing 0x00 at 100KHz I2C)
    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, 0, ACK_CHECK_DIS);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, ATMEL_RESPONSE_TIMEOUT_GENERIC / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }

    // 2. check for successful message sending
    if (ret != ESP_OK)
    {
        return ret;
    }

    // 3. some delay. 1500 uSeconds is minimum and known as "Wake High Delay to Data Comm." tWHI, and SDA must be high during this time.
    atecc508a_delay(2);

    // 4. Try to read back from the ATECC508A (1 byte + status + CRC)
    uint8_t response[1 + 3] = {};
    ret = atecc508a_receive(response, sizeof(response));
    if (ret != ESP_OK)
    {
        return ret;
    }

    // 5. check crc and wake up value
    ret = atecc508a_check_crc(response, sizeof(response));
    if (ret != ESP_OK)
    {
        return ret;
    }

    // 6. check if the received response matches what is expected
    if (response[1] != ATECA_RESPONSE_WAKE)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t atecc508a_sleep()
{
    // 1. send sleep command to chip
    return ESP_OK;
    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (ATECC508A_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x01, ACK_CHECK_DIS);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_PORT, cmd, ATMEL_RESPONSE_TIMEOUT_SLEEP / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }

    // 2. check for successful sending of message
    /*
    if (ret != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error entering sleep mode");
    }
    */
    return ESP_OK;
}

esp_err_t atecc508a_idle()
{
    esp_err_t ret = ESP_FAIL;

    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (ATECC508A_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x02, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, ATMEL_RESPONSE_TIMEOUT_GENERIC / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }

    if (ret != ESP_OK)
    {
        ESP_LOGE(LOG_TAG, "Error entering idle mode");
    }
    return ret;
}
#endif