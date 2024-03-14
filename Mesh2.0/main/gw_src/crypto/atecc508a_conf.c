/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a_conf.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip for setting up and referring to 
 * configuration zones
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "includes/SpacrGateway_commands.h"
#define LOG_TAG "atecc508a_conf"

uint8_t atecc508a_config[128];

esp_err_t atecc508a_read_config_zone()
{
    ESP_ERROR_CHECK(atecc508a_read(ATECC508A_ZONE_CONFIG, ATECC508A_ADDRESS_CONFIG_READ_BLOCK_0, atecc508a_config, 32));

    ESP_ERROR_CHECK(atecc508a_read(ATECC508A_ZONE_CONFIG, ATECC508A_ADDRESS_CONFIG_READ_BLOCK_1, atecc508a_config + 32, 32));

    ESP_ERROR_CHECK(atecc508a_read(ATECC508A_ZONE_CONFIG, ATECC508A_ADDRESS_CONFIG_READ_BLOCK_2, atecc508a_config + 64, 32));

    ESP_ERROR_CHECK(atecc508a_read(ATECC508A_ZONE_CONFIG, ATECC508A_ADDRESS_CONFIG_READ_BLOCK_3, atecc508a_config + 96, 32));

    ESP_LOGI(LOG_TAG, "Config Zone:");
    ESP_LOG_BUFFER_HEX_LEVEL(LOG_TAG, atecc508a_config, 128, ESP_LOG_DEBUG);

    return ESP_OK;
}

esp_err_t atecc508a_is_configured(uint8_t *is_configured)
{
    ESP_LOGI(LOG_TAG, "Current config status:");
    ESP_LOGI(LOG_TAG, "  dataOTPLockStatus = 0x%02X", atecc508a_config[86]);
    ESP_LOGI(LOG_TAG, "  configLockStatus = 0x%02X", atecc508a_config[87]);
    ESP_LOGI(LOG_TAG, "  slot0LockStatus = 0x%02X", atecc508a_config[88] & 1);

    // dataOTPLockStatus = config[86], then set according to status (0x55=UNlocked, 0x00=Locked)
    // configLockStatus = config[87], then set according to status (0x55=UNlocked, 0x00=Locked)
    // slot0LockStatus = config[88], then set according to status (0x55=UNlocked, 0x00=Locked)
    if (atecc508a_config[86] == 0x00 && atecc508a_config[87] == 0x00 && (atecc508a_config[88] & 1) == 0x00)
    {
        *is_configured = 1;
    }
    else
    {
        *is_configured = 0;
    }

    return ESP_OK;
}

esp_err_t atecc508a_write_config()
{

    return ESP_OK;
}

esp_err_t atecc508a_configure()
{
    ESP_ERROR_CHECK(atecc508a_write_config());

    // ESP_ERROR_CHECK(atecc508a_lock(LOCK_MODE_ZONE_CONFIG));

    ESP_ERROR_CHECK(atecc508a_create_key_pair());

    // ESP_ERROR_CHECK(atecc508a_lock(LOCK_MODE_ZONE_DATA_AND_OTP));

    // ESP_ERROR_CHECK(atecc508a_lock(LOCK_MODE_SLOT0));

    return ESP_OK;
}
#endif