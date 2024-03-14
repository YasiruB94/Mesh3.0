/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: cence_crypto_chip_provision.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used to provision a crypto chip to the requirements needed
 * to achieve proper handshake with mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/crypto/cence_crypto_chip_provision.h"
static const char *TAG = "ATMEL508";


/**
 * @brief Does a full provision of an ATMEL508 chip.
 *
 * @return ESP_OK for a successful operation
 */
esp_err_t ATMEL508_provision_chip()
{
    esp_err_t result = ESP_FAIL;
    ESP_LOGI(TAG, "ATMEL508_provision_chip");
    // CONFIGURATION ZONE
    // 1. check the state of the configuration zone
    esp_err_t lock_state = ATMEL508_check_configuration_zone_state();
    switch (lock_state)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "configuration zone is locked!");
        result = ESP_OK;
        break;
    case ESP_FAIL:
        ESP_LOGI(TAG, "configuration zone is NOT locked!");
        result = ESP_OK;
        break;
    case ESP_ERR_NOT_FOUND:
        ESP_LOGE(TAG, "ERROR determining configuration zone!");
        return result;
        break;
    default:
        return result;
        break;
    }

    if (lock_state == ESP_FAIL)
    {
        // the config slots are not locked
        result = Verify_Keys();
        if (ESP_OK != result)
        {
            ESP_LOGE(TAG, "There are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
            return result;
        }
        else
        {
            ESP_LOGI(TAG, "KEYS OK!");
        }
        // personalize configuration zone
        result = ATMEL508_personalize_configuration_zone();
        if (ESP_OK != result)
        {
            ESP_LOGI(TAG, "Personalize Configuration Zone failed with status %s", esp_err_to_name(result));
            return result;
        }
        else
        {
            ESP_LOGI(TAG, "Personalize Configuration Zone: %s", esp_err_to_name(result));
        }
    }

    // DATA ZONE
    // personalize data zone
    result = ATMEL508_personalize_data_zone();
    if (ESP_OK != result)
    {
        ESP_LOGI(TAG, "Personalize Data Zone failed with status %s", esp_err_to_name(result));
    }
    else
    {
        ESP_LOGI(TAG, "Personalize Data Zone: %s", esp_err_to_name(result));
    }

    return result;
}



/**
 * @brief     Generates a random number from the security chip.
 * @note      The rendom number will be in the form of 00 FF 00 FF ... if the configuration zones are not locked
 * @param[in] random_num pointer to the location where the random number will be copied
 * @return    ESP_OK if the operation is successful, if not, returns ESP_FAIL
 */
esp_err_t ATMEL508_get_random_number(uint8_t *random_num)
{
    esp_err_t result = ESP_FAIL;
    result = atecc508a_response(random_num, ATCA_RSP_SIZE_32, ATCA_RANDOM, 0x00, 0x000, NULL, 0, ATMEL_READ_DATA_DELAY_GENERIC);
    
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Error generating random number");
    }

    return result;
}


/**
 * @brief     Generates a random number from the security chip.
 * @note      The rendom number will be in the form of 00 FF 00 FF ... if the configuration zones are not locked
 * @return    ESP_OK if the configuration zone is locked, ESP_FAIL if it is not locked, ESP_ERR_NOT_FOUND if reading the state failed
 */
esp_err_t ATMEL508_check_configuration_zone_state()
{
    esp_err_t result         = ESP_ERR_NOT_FOUND;
    uint8_t data[4U]         = {0};
    uint8_t byte             = 87U;
    const uint8_t block      = byte / ATCA_BLOCK_SIZE;
    const uint8_t offset     = (byte - (block * ATCA_BLOCK_SIZE) - 1) / ATCA_WORD_SIZE;
    const uint8_t word_index = (byte - (block * ATCA_BLOCK_SIZE) - (offset * ATCA_WORD_SIZE));

    ATMEL_RW_CTX_t rw_ctx =
        {
            .zone       = ATCA_ZONE_CONFIG,
            .slot       = 0U,
            .block      = block,
            .offset     = offset,
            .data       = data,
            .len        = sizeof(data),
        };

    result = ATMEL_Read_Data(&rw_ctx);
    atecc508a_sleep();

    if (result == ESP_OK)
    {
        //read data successful
        if (0x55 == *(data + word_index))
        {
            //configuration zone NOT locked
            result = ESP_FAIL;
        }
        else if (0x00 == *(data + word_index))
        {
            //configuration zone locked
            result = ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Error determining Configuration Zone state");
            result = ESP_ERR_NOT_FOUND;
        }
    }
    else
    {
        //read data failed
        result = ESP_ERR_NOT_FOUND;
    }

    return result;
}

/**
 * @brief Verifies that actual keys are contained in the cn_keys_bb.h file.
 *
 * @return ATCA_SUCCESS if keys are there
 */
esp_err_t Verify_Keys(void)
{
    if (0 == memcmp(data_0, data_ZEROS, 32U))
    {
        return ESP_FAIL;
    }
    if (0 == memcmp(data_2, data_ZEROS, 32U))
    {
        return ESP_FAIL;
    }
    if (0 == memcmp(data_3, data_ZEROS, 32U))
    {
        return ESP_FAIL;
    }
    if (0 == memcmp(data_9, data_ZEROS, 32U))
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}


/**
 * @brief updates the configuration zone of the ATMEL508 chip and then locks it.
 *
 * @return ESP_OK if successful
 */
esp_err_t ATMEL508_personalize_configuration_zone()
{
    static uint8_t config[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0 - 15
        0xC0, 0x00, 0xAA, 0x00, 0x81, 0x80, 0xc2, 0x30, 0x82, 0x80, 0x81, 0x80, 0x11, 0x00, 0x10, 0x80, // 16 - 31
        0xc2, 0x42,                                                                                     // 32 - 33
        0x87, 0x62,                                                                                     // SlotConfig slot 7 Bytes 34 - 35
        0x11, 0x00, 0x91, 0x80,                                                                         // 36 - 39
        0xD2, 0x42,                                                                                     // SlotConfig slot 10 Bytes40 - 41
        0xD2, 0x42,                                                                                     // SlotConfig slot 11 Bytes42 - 43
        0xD2, 0x42,                                                                                     // SlotConfig slot 12 Bytes44 - 45
        0xD2, 0x42,                                                                                     // SlotConfig slot 13 Bytes46 - 47
        0xD2, 0x42,                                                                                     // SlotConfig slot 14 Bytes48 - 49
        0x87, 0x42,                                                                                     // SlotConfig slot 15 Bytes50 - 51
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         // 52 - 63
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 64 - 79
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x55, 0x55, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 80 - 95
        /**keyConfig bit start*/
        0xbc, 0x02, 0x1C, 0x00, 0x3C, 0x00, 0xbc, 0x02, 0x3C, 0x00, 0x3c, 0x00, 0x3c, 0x00, // 96 - 109
        0x93, 0x02,                                                                         // KeyConfig slot 7 bytes 110 - 111
        0x3c, 0x00, 0x30, 0x00, 0x1C, 0x00, 0x3c, 0x00, 0x30, 0x00, 0x3c, 0x00, 0x30, 0x00, // 112 125
        0xB3, 0x02                                                                          // KeyConfig slot 15 bytes 126 - 127
        /**keyConfig bit end*/
    };
    uint8_t crc[2]      = {0};
    esp_err_t result    = ESP_FAIL;
    ATMEL_Add_CRC(128, config, crc);

    do
    {
        if (ESP_OK != (result = ATMELPROV_Set_Configuration_Zone(config)))
        {
            ESP_LOGE(TAG, "Error: ATMELPROV_Set_Configuration_Zone");
            break;
        }
        if (ESP_OK != (result = ATMELPROV_Validate_Configuration_Zone(config)))
        {
            ESP_LOGE(TAG, "Error: ATMELPROV_Validate_Configuration_Zone");
            break;
        }
        // load current value for data bytes which were not writable so a CRC can be generated
        uint8_t read_data[32] = {0};
        ATMEL_RW_CTX_t rw_ctx =
            {
                .zone       = ATCA_ZONE_CONFIG,
                .slot       = 0U,
                .block      = 0U,
                .offset     = 0U,
                .data       = read_data,
                .len        = 32U,
            };
        if (ESP_OK != (result = ATMEL_Read_Data(&rw_ctx)))
        {
            ESP_LOGE(TAG, "Error reading data from ATMEL chip");
            break;
        }
        memcpy(config, read_data, 16U);

        ATMEL_RW_CTX_t rw_ctx2 =
            {
                .zone       = ATCA_ZONE_CONFIG,
                .slot       = 0U,
                .block      = 2U,
                .offset     = 0U,
                .data       = read_data,
                .len        = 32U,
            };
        if (ESP_OK != (result = ATMEL_Read_Data(&rw_ctx2)))
        {
            ESP_LOGE(TAG, "Error reading data from ATMEL chip");
            break;
        }
        memcpy(&config[84], &read_data[20], 8U);

        uint8_t crc2[2] = {0};
        ATMEL_Add_CRC(128, config, crc2);

        // do the lock here
        result = ATMELPROV_Lock(ATCA_ZONE_CONFIG, 0U, crc2);
        if (ESP_OK != result)
        {
            ESP_LOGE(TAG, "Error locking configuration zone");
            break;
        }

    } while (0);

    return result;
}

/**
 * @brief updates the data zone of the ATMEL508 chip and then locks it.
 *
 * @return ESP_OK if successful
 */
esp_err_t ATMEL508_personalize_data_zone()
{
    esp_err_t result                            = ESP_FAIL;
    static const uint16_t PRODUCT_ID            = CENSE_PRODUCT_ID_CABINET;
    static const uint16_t MODEL_NUMBER          = CENSE_MODEL_NUMBER_CABINET;
    static const uint8_t HARDWARE_MAJAOR_VER    = 0x01;
    static const uint8_t HARDWARE_MINOR_VER     = 0x01;
    static const uint16_t HARDWARE_BUILD_NUMBER = 0x0000;

    const uint8_t data_5[36]                    = {(PRODUCT_ID & 0xFF), ((PRODUCT_ID >> 8) & 0xFF), (MODEL_NUMBER & 0xFF), ((MODEL_NUMBER >> 8) & 0xFF), HARDWARE_MAJAOR_VER, HARDWARE_MINOR_VER, (HARDWARE_BUILD_NUMBER & 0xFF), ((HARDWARE_BUILD_NUMBER >> 8) & 0xFF), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t data_0_CRC[]                  = {0xcf, 0xdc};
    const uint8_t data_2_CRC[]                  = {0x9a, 0x47};
    const uint8_t data_3_CRC[]                  = {0x5b, 0x2c};
    const uint8_t data_9_CRC[]                  = {0x99, 0xa0};
    uint8_t crc[2]                              = {0};
    uint16_t slot_lock;

    do
    {
        if (ESP_OK != (result = ATMELPROV_Get_Data_Slot_Locked_Status(&slot_lock)))
        {
            ESP_LOGE(TAG, "Error determining the locked status of the slots");
            break;
        }
        //
        // SLOT 0
        // ///////////////
        if (slot_lock & (0x01 << 0))
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            ESP_LOGI(TAG, "Writing Data Zone Slot 0...");
            if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 0U, 0U, 0U, data_0, 32U)))
            {
                ESP_LOGE(TAG, "Failed to Write Data Zone Slot 0, Error %s.", esp_err_to_name(result));
                break;
            }
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 0U, data_0_CRC)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot 0, Error %s.", esp_err_to_name(result));
                break;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Data Zone - Slot 0 already locked.  Skipping configuration of this slot.");
        }

        //
        // SLOT 2
        // ///////////////
        if (slot_lock & (0x01 << 2))
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            ESP_LOGI(TAG, "Writing Data Zone Slot 2...");
            if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 2U, 0U, 0U, data_2, 32U)))
            {
                ESP_LOGE(TAG, "Failed to Write Data Zone Slot 2, Error %s.", esp_err_to_name(result));
                break;
            }
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 2U, data_2_CRC)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot 2, Error %s.", esp_err_to_name(result));
                break;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Data Zone - Slot 2 already locked.  Skipping configuration of this slot.");
        }

        //
        // SLOT 3
        // ///////////////
        if (slot_lock & (0x01 << 3))
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            ESP_LOGI(TAG, "Writing Data Zone Slot 3...");
            if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 3U, 0U, 0U, data_3, 32U)))
            {
                ESP_LOGE(TAG, "Failed to Write Data Zone Slot 3, Error %s.", esp_err_to_name(result));
                break;
            }
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 3U, data_3_CRC)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot 3, Error %s.", esp_err_to_name(result));
                break;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Data Zone - Slot 3 already locked.  Skipping configuration of this slot.");
        }

        //
        // SLOT 5
        // ///////////////
        if (slot_lock & (0x01 << 5))
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            ESP_LOGI(TAG, "Writing Data Zone Slot 5...");
            if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 5U, 0U, 0U, data_5, 32U)))
            {
                ESP_LOGE(TAG, "Failed to Write Data Zone Slot 5, Error %s.", esp_err_to_name(result));
                break;
            }
            ATMEL_Add_CRC(32U, data_5, crc);
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 5U, crc)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot 5, Error %s.", esp_err_to_name(result));
                break;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Data Zone - Slot 5 already locked.  Skipping configuration of this slot.");
        }
        //
        // SLOT 9
        // ///////////////
        if (slot_lock & (0x01 << 9))
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            ESP_LOGI(TAG, "Writing Data Zone Slot 9...");
            for (uint32_t i = 0; i < 3; i++)
            {
                if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 9U, i, 0U, &data_9[i * 32], 32U)))
                {
                    ESP_LOGE(TAG, "Failed to Write Data Zone Slot 9, Error %s.", esp_err_to_name(result));
                    break;
                }
            }
            if (ESP_OK != result)
            {
                break;
            }
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 9U, data_9_CRC)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot9, Error %s.", esp_err_to_name(result));
                break;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Data Zone - Slot 9 already locked.  Skipping configuration of this slot.");
        }
        //
        // SLOT 15
        // ///////////////
        if ((slot_lock & (0x01 << 15)) || (slot_lock & (0x01 << 14)))
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            uint8_t public_key[64];
            ESP_LOGI(TAG, "Generating Private Key in Data Zone Slot 15...");
            if (ESP_OK != (result = ATMEL_Gen_Key(15U, GENKEY_MODE_PRIVATE_KEY_GENERATE, false, public_key)))
            {
                ESP_LOGE(TAG, "Failed to Generate Private key in Data Zone Slot 15, Error %s.", esp_err_to_name(result));
                break;
            }
            // write public key
            ESP_LOGI(TAG, "Writing public key to Data Zone Slot 14.");
            if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 14U, 0U, 0U, public_key, 32U)))
            {
                ESP_LOGE(TAG, "Failed to Write Public key in Data Zone Slot 14, Block 0, Error %s.", esp_err_to_name(result));
                break;
            }
            ESP_LOGI(TAG, "Writing public key to Data Zone Slot 14.");
            if (ESP_OK != (result = ATMEL_Write_Data(ATCA_ZONE_DATA, 14U, 1U, 0U, &public_key[32U], 32U)))
            {
                ESP_LOGE(TAG, "Failed to Write Public key in Data Zone Slot 14, Block 1, Error %s.", esp_err_to_name(result));
                break;
            }
            ATMEL_Add_CRC(64U, public_key, crc);
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 14U, crc)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot14, Error %s.", esp_err_to_name(result));
                break;
            }
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA_SLOT, 15U, NULL)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone Slot15, Error %s.", esp_err_to_name(result));
                break;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Data Zone - Slot 14 and or 15 already locked.  Skipping configuration of this slot.");
        }

        bool data_zone_locked = false;
        if (ESP_OK != (result = ATMEL_Is_Zone_Locked(ATCA_ZONE_DATA, &data_zone_locked)))
        {
            ESP_LOGE(TAG, "Failed to Read Data Zone Lock Status, Error %s.", esp_err_to_name(result));
            break;
        }
        if (false == data_zone_locked)
        {
            if (ESP_OK != (result = Verify_Keys()))
            {
                ESP_LOGE(TAG, "there are no keys stored in cn_keys_bb.h file, update with actual keys and run again.");
                break;
            }
            ESP_LOGI(TAG, "Locking Data Zone...");
            // lock data zone
            if (ESP_OK != (result = ATMELPROV_Lock(LOCK_ZONE_DATA, 0U, (uint8_t *)NULL)))
            {
                ESP_LOGE(TAG, "Failed to Lock Data Zone, Error %s.", esp_err_to_name(result));
                break;
            }
            ESP_LOGI(TAG, "Preparing data zones for general data storage.");
            // clear slot 10 data (Cabinet Metrics)
            const uint8_t ZEROS_32[32] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};

            if (ESP_OK != (result = ATMEL_Encrypted_Write(ATCA_ZONE_DATA, 10U, 0U, ZEROS_32)))
            {
                ESP_LOGE(TAG, "Failed to erase data slot 10.");
                break;
            }
            if (ESP_OK != (result = ATMEL_Encrypted_Write(ATCA_ZONE_DATA, 10U, 1U, ZEROS_32)))
            {
                ESP_LOGE(TAG, "Failed to erase data slot 10.");
                break;
            }
        }

    } while (0);

    return result;
}

#endif