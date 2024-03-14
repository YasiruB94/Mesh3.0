/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: crypto.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used to provision a crypto chip, all crypto related
 * functions
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "gw_includes/crypto/crypto.h"

static const char *TAG = "crypto";
static uint8_t code_auth_key_slot = 0U;

/** @brief This function calculates CRC given raw data, puts the CRC to given pointer
 *
 * @param[in] length size of data not including the CRC byte positions
 * @param[in] data pointer to the data over which to compute the CRC
 * @param[out] crc pointer to the place where the two-bytes of CRC will be placed
 */
static void Get_Address(const uint8_t zone, const uint8_t slot, const uint8_t block, uint8_t offset, uint16_t *address)
{
    uint8_t memzone = zone & 0x03;
    *address = 0;
    // Mask the offset
    offset = offset & (uint8_t)0x07;
    if ((memzone == ATCA_ZONE_CONFIG) || (memzone == ATCA_ZONE_OTP))
    {
        *address = block << 3;
        *address |= offset;
    }
    else
    { // ATCA_ZONE_DATA
        *address = slot << 3;
        *address |= offset;
        *address |= block << 8;
    }
}

/**
 * @brief Calculates and sets the ATMEL CRC for the supplied data
 *
 * @param[in]  length 	Length of the data buffer
 * @param[in]  data		Data buffer which the CRC is to be based on
 * @param[out] crc		A 2 Byte CRC value in LSB
 */
void ATMEL_Add_CRC(const uint8_t length, const uint8_t *data, uint8_t *crc)
{
    uint8_t counter;
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t shift_register;
    uint8_t data_bit, crc_bit;

    for (counter = 0; counter < length; counter++)
    {
        for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
        {
            data_bit = (data[counter] & shift_register) ? 1 : 0;
            crc_bit = crc_register >> 15;
            crc_register <<= 1;
            if (data_bit != crc_bit)
                crc_register ^= polynom;
        }
    }
    crc[0] = (uint8_t)(crc_register & 0x00FF);
    crc[1] = (uint8_t)(crc_register >> 8);
}

/**
 * @brief Calculates and adds the CRC to the packet.
 *
 * @param[in] packet The packet to calculate CRC for.
 */
void ATMEL_Calculate_CRC(ATCAPACKET_t *packet)
{
    uint8_t length;
    uint8_t *crc;

    length = packet->txsize - ATCA_CRC_SIZE;
    crc = &(packet->txsize) + length;

    // add crc to packet
    ATMEL_Add_CRC(length, &(packet->txsize), crc);
}

/**
 * @brief Used in device authentication to generate a response digest to a challenge which has been presented.
 *  The device is put to sleep via #ATMELUTIL_Sleep after executing this command.
 * @example: During the handshaking process between the Control MCU (host) and the Driver or Gateway (peripheral device), the Control MCU
 *          will generate a challenge and send it to the peripheral device.  The peripheral device uses this function to generate
 *          the response which it sends back to the Control MCU for verification.
 *          The response that is generated will make use of the device serial number, so it is important for the host to have
 *          the serial number of the peripheral device in order to be able to properly validate it.
 * @param[in] challenge         The 32 byte Input challenge for which to generate the response for.
 * @param[in] device_auth_key   The key slot where the device authentication key is stored
 * @param[in] response          The 32 byte output response will be stored here
 * @return status of the operation
 */
esp_err_t ATMEL_Challenge_MAC(uint8_t *challenge, const uint8_t device_auth_key, uint8_t *const response)
{
    esp_err_t result = ESP_OK;
    uint8_t response_buffer[ATCA_RSP_SIZE_32];

    result = Authorize_MAC();
    if (result != ESP_OK)
    {
        return result;
    }

    result = atecc508a_response(response_buffer, ATCA_RSP_SIZE_32, ATCA_MAC, 0x40, device_auth_key, challenge, 39, ATMEL_READ_DATA_DELAY_GENERIC);
    if (result != ESP_OK)
    {
        return result;
    }

    memcpy(response, response_buffer, ATMEL_MAC_SIZE);
    return result;
}

/**
 * @brief Validates an HMAC for a given message
 * @note The device is put to sleep via #ATMELUTIL_Sleep after executing this command.
 * @param[in] message   The message which the HMAC is based on.  A message digest will be created and used
 *                      as part of the HMAC calculation
 * @param[in] length    The length of the message not including the HMAC
 * @param[in] key_slot  The key slot for the key to use in the HMAC calculation
 * @param[in] hmac      The HMAC to validate.  IF CN_NULL then it is assumed to be at the end of the message
 * @return ATCA_SUCCESS if HMAC is valid.  Returns ATCA_CHECKMAC_VERIFY_FAILED if hmac is not valid.  Can return
 *          other #ATCA_STATUS values if an error occurs
 */
esp_err_t ATMEL_Validate_HMAC(const uint8_t *message, const uint16_t length, const uint8_t key_slot, const uint8_t *hmac)
{
    if (unsuccessful_handshake_attempts > 10)
    {
        error_handler("Too many unsuccessful handshake attempts", ESP_FAIL);
    }
    if (NULL == hmac)
    {
        hmac = (uint8_t *)(message + length);
    }
    uint8_t calc_hmac[ATMEL_HMAC_SIZE] = {0};

    esp_err_t result = ATMEL_HMAC(message, length, key_slot, calc_hmac);

    if (result == ESP_OK)
    {
        // during handshake 02, the last two bytes are missing in the hmac coming from the mainboard.
        // so we ignore the last two bytes when comparing the two hmacs
        if (memcmp(hmac, calc_hmac, ATMEL_HMAC_SIZE - 2) != 0)
        {
            ESP_LOGE(TAG, "HMAC comparison failed");
            printf("HMAC\t\t: ");
            for (uint8_t i = 0; i < ATMEL_HMAC_SIZE; i++)
            {
                printf("%02X ", hmac[i]);
            }
            printf("\n");
            printf("CALC HMAC\t: ");
            for (uint8_t i = 0; i < ATMEL_HMAC_SIZE; i++)
            {
                printf("%02X ", calc_hmac[i]);
            }
            printf("\n");
            result = ESP_FAIL;
        }
    }
    atecc508a_sleep();
    return result;
}

/**
 * @brief Calculates an HMAC based on the supplied message.  The message is first hashed
 *        and the digest is used along with the key at the supplied key slot.
 * @note The device is put to sleep via #ATMELUTIL_Sleep after executing this command.
 * @param[in]  message  An arbitrary length message to perform the HMAC calculation on
 * @param[in]  length   The length of the message, exclude the 32bytes if hmac is NULL
 * @param[in]  key_slot The key slot for the key to use in the HMAC calculation
 * @param[out] hmac     The 32 bit output HMAC digest will be stored here.  If CN_NULL is
 *                      passed in then the HMAC digest will be added to the end of the message
 *                      variable at position message[length] - ensure there is 32 bytes available.
 * @return status of operation
 */
esp_err_t ATMEL_HMAC(const uint8_t *const message, const uint32_t length, const uint8_t key_slot, uint8_t *hmac)
{
    esp_err_t result;
    uint8_t digest[ATMEL_HASH_SIZE] = {0};

    // 1. create the nonce digest
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, message, length);
    mbedtls_sha256_finish_ret(&ctx, digest);

    // 2. check authorize mac
    result = Authorize_MAC();
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Authorize mac error");
        return result;
    }

    // 3. nonce passthrough. the above calculated sha256 digest is sent to the chip
    result = ATMEL_Nonce_Passthrough(digest);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Nonce passthrough error");
        return result;
    }

    // 4. get the HMAC response from chip
    uint8_t response[ATCA_RSP_SIZE_32];
    result = atecc508a_response(response, ATCA_RSP_SIZE_32, ATCA_HMAC, 0x04, key_slot, NULL, 0, ATMEL_READ_DATA_DELAY_HMAC);
    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "HMAC read error");
        return result;
    }

    // 5. check if the response is an error code
    if (response[0] == ATECA_RESPONSE_ERROR || response[0] == ATECA_RESPONSE_EXEC_ERROR)
    {
        ESP_LOGE(TAG, "HMAC Response error code: %02X", response[0]);
        return ESP_FAIL;
    }

    // 6. check if the received message length is of expected length
    if (response[0] != ATCA_RSP_SIZE_32)
    {
        ESP_LOGE(TAG, "HMAC Response size mismatch");
        return ESP_FAIL;
    }

    // 7. copy the received data to the relevant buffer
    memcpy(hmac, response + 1, ATCA_RSP_SIZE_32);
    atecc508a_sleep();

    return result;
}

/**
 * @brief  Authorizes the use of #ATMEL_Derive_Key using the code authorization key
 * @return status of operation
 */
esp_err_t Authorize_MAC()
{
    esp_err_t status;
    uint8_t serial[ATMEL_SERIAL_LENGTH] = {0};

    // 1. get the serial ID
    status = ATMEL_Get_Serial_ID(serial);
    if (status != ESP_OK)
    {
        return status;
    }
    uint8_t salt[ATMEL_SALT_SIZE] = {0};
    uint8_t nonce[ATMEL_NONCE_SIZE] = {0};

    // 2. generate the nonce value
    status = ATMEL_Nonce_Random(salt, nonce);
    if (status != ESP_OK)
    {
        return status;
    }

    // 3. calculate authorization MAC (initialize variable first)
    memset(salt, 0, sizeof(salt));
    uint8_t key[ATMEL_KEY_SIZE];
    mbedtls_sha256_context ctx;
    ATMELUTIL_Get_Code_Authorization(key);

    // 4. i dont know what is happening here. LOL
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, key, ATMEL_KEY_SIZE);
    mbedtls_sha256_update_ret(&ctx, nonce, ATMEL_NONCE_SIZE);
    mbedtls_sha256_update_ret(&ctx, salt, 4U);
    SHA256_Zero(&ctx, 8);
    mbedtls_sha256_update_ret(&ctx, &salt[4], 3U);
    mbedtls_sha256_update_ret(&ctx, &serial[8], 1U);
    mbedtls_sha256_update_ret(&ctx, &salt[7], 4U);
    mbedtls_sha256_update_ret(&ctx, serial, 2U);
    mbedtls_sha256_update_ret(&ctx, &salt[11], 2U);

    // 5. send check mac command to chip to verify a MAC calculated on another CryptoAuthentication device
    uint8_t response[ATCA_RSP_SIZE_MIN];
    ATCAPACKET_t packet = {0};
    mbedtls_sha256_finish_ret(&ctx, &packet.data[CHECKMAC_CLIENT_CHALLENGE_SIZE]);
    memcpy(&packet.data[CHECKMAC_CLIENT_CHALLENGE_SIZE + CHECKMAC_CLIENT_RESPONSE_SIZE], salt, CHECKMAC_OTHER_DATA_SIZE);
    status = atecc508a_response(response, ATCA_RSP_SIZE_MIN, ATCA_CHECKMAC, 0x01, 2U, packet.data, 85, ATMEL_READ_DATA_DELAY_GENERIC);
    if (status != ESP_OK)
    {
        return status;
    }

    // 6. check if the size of the response received is the expected size
    if (response[0] != ATCA_RSP_SIZE_MIN)
    {
        return ESP_FAIL;
    }

    // 7. check if the chip responded with a positive note (0 == PASS, 1 == FAIL)
    if (response[1] != ATECA_RESPONSE_SUCCESS)
    {
        return ESP_FAIL;
    }

    return status;
}

/**
 * @brief Executes a pass-through Nonce command to initialize tempKey to the supplied value
 * @param[in] tempKey A 32 byte value to store in the TempKey
 * @return Status of operation
 */
esp_err_t ATMEL_Nonce_Passthrough(uint8_t *tempkey)
{
    uint8_t response[ATCA_RSP_SIZE_MIN];
    esp_err_t result = ESP_OK;
    // 1. send the tempkey digest to the chip
    result = atecc508a_response(response, ATCA_RSP_SIZE_MIN, ATCA_NONCE, 0x03, 0, tempkey, 77, ATMEL_READ_DATA_DELAY_GENERIC);
    if (result != ESP_OK)
    {
        return result;
    }

    // 2. check if the response is an error message
    if (response[0] == ATECA_RESPONSE_ERROR || response[0] == ATECA_RESPONSE_EXEC_ERROR)
    {
        return ESP_FAIL;
    }

    // 3. check if the received data length is of the expected size
    if (response[0] != ATCA_RSP_SIZE_MIN)
    {
        return ESP_FAIL;
    }

    // 4. check if the chip responded with a positive note (0 == PASS, 1 == FAIL)
    if (response[1] != ATECA_RESPONSE_SUCCESS)
    {
        return ESP_FAIL;
    }

    return result;
}

/**
 * @brief Generates the key from the slot data. We don't
 * want to store the real data
 * @param Key A buffer that is at least ATMEL_KEY_SIZE
 */
void ATMELUTIL_Get_Code_Authorization(uint8_t *const key)
{
    static const uint8_t slot_data[] =
        {
            /*Forced in flash*/
            0x69, 0xf6, 0x0f, 0x46, 0xd0, 0xf4, 0x02,
            0xb7, 0x8e, 0x25, 0x2c, 0x20, 0x94, 0x3c,
            0xff, 0xac, 0x1f, 0x03, 0x73, 0x7b, 0x99,
            0x43, 0x2f, 0xa5, 0x84, 0x68, 0xbd, 0x68,
            0x64, 0xa4, 0x1f, 0x1f};

    uint16_t i = 32;

    while (i--)
    {
        key[i] = slot_data[i] ^ 0xA3;
    }
}

/**
 * Generates a Nonce from supplied salt and a ATMEL508A randomly generated number
 * The Nonce is created by doing a SHA 256 of the following
 * - RandOut from ATMEL508A (result of Nonce Command)
 * - salt
 * - Nonce Opcode   (0x16)
 * - Nonce Mode     (0x00)
 * - Zero Byte      (0x00)
 * @param[in]  salt     A 20-byte random salt
 * @param[out] nonce    The generated nonce will be stored here in a 32byte array
 * @return status of operation
 */
esp_err_t ATMEL_Nonce_Random(const uint8_t *const salt, uint8_t *const nonce)
{
    uint8_t response[ATCA_RSP_SIZE_32];

    ATCAPACKET_t packet = {0};
    packet.word_address = 0x03;
    packet.opcode = ATCA_NONCE;
    packet.param1 = 0x0000;
    packet.param2 = 0;
    memcpy(packet.data, salt, ATMEL_SALT_SIZE);

    // 1. get the nonce value from chip
    esp_err_t result = atecc508a_response(response, ATCA_RSP_SIZE_32, packet.opcode, packet.param1, packet.param2, packet.data, 28, ATMEL_READ_DATA_DELAY_GENERIC);
    if (result != ESP_OK)
    {
        return result;
    }

    // 2. check for error commands
    if (response[0] == ATECA_RESPONSE_ERROR || response[0] == ATECA_RESPONSE_EXEC_ERROR)
    {
        ESP_LOGE(TAG, "ATMEL_Nonce error!");
        return ESP_FAIL;
    }

    // 3. check if the received data is of the expected length
    if (response[0] != ATCA_RSP_SIZE_32)
    {
        return ESP_FAIL;
    }

    // 4. copy the received data to the required buffer
    memcpy(packet.data, response + 1, ATCA_RSP_SIZE_32);

    // 5. generate the nonce using sha256
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, packet.data, ATMEL_NONCE_SIZE);
    mbedtls_sha256_update_ret(&ctx, salt, ATMEL_SALT_SIZE);
    mbedtls_sha256_update_ret(&ctx, &packet.opcode, 1U);
    mbedtls_sha256_update_ret(&ctx, &packet.param1, 1U);
    SHA256_Zero(&ctx, 1U);
    mbedtls_sha256_finish_ret(&ctx, nonce);

    return result;
}

/**
 * @brief Helper function for hashing with zeros of a given size
 * @param ctx A valid sha256 context
 * @param count The count size
 */
void SHA256_Zero(mbedtls_sha256_context *ctx, const uint16_t count)
{
    const uint8_t *zeros = OS_Get_512_Zeros();
    mbedtls_sha256_update_ret(ctx, zeros, count);
}

/**
 * @brief Give the caller the pointer to flash memory space
 * of 512byte all zeroed out. This method is used in MBED TLS ports
 * as well as any method that just need zero to do some pre-calculation
 * on such as hash filling etc. The caller must not try to write
 * to this memory space as it's in flash
 * @return 512 byte zero.
 */
const uint8_t *OS_Get_512_Zeros(void)
{
    static const uint8_t flash_zero[512] = {0};
    return flash_zero;
}

/**
 * @brief Returns the unique 9 byte serial number of the device.
 * @attention This method must be called at board bring up to cache the serial number.
 * If this is not done undefined timing behavior can occur with the API normal usage
 * @note The device is put to sleep via #ATMELUTIL_Sleep after executing this command.
 * @param[out] buffer A buffer large enough to fit the 9 byte serial number. Optional
 * @param[in] timeout_ticks RTOS timeout in ticks
 * @return  NULL --> Failure Else a pointer to the static allocated memory
 * that holds the serial number
 */
esp_err_t ATMEL_Get_Serial_ID(uint8_t *const buffer)
{
    static uint8_t serial_cache[ATMEL_SERIAL_LENGTH] = {0};
    uint8_t word[4];
    esp_err_t result = ESP_OK;

    ATMEL_RW_CTX_t rw_ctx =
        {
            .zone = 0x00,
            .slot = 0,
            .block = 0,
            .offset = 0,
            .data = word,
            .len = sizeof(word),
        };

    // collect the serial ID of the chip by calling the read function thrice (serial ID is stored in three different locations)
    result = ATMEL_Read_Data(&rw_ctx);
    if (result != ESP_OK)
    {
        return result;
    }
    uint8_t *pserial = serial_cache;
    memcpy(pserial, word, sizeof(word));
    rw_ctx.offset = 2;
    pserial += 4;

    result = ATMEL_Read_Data(&rw_ctx);
    if (result != ESP_OK)
    {
        return result;
    }
    memcpy(pserial, word, sizeof(word));
    rw_ctx.offset = 3;
    pserial += 4;

    result = ATMEL_Read_Data(&rw_ctx);
    if (result != ESP_OK)
    {
        return result;
    }
    memcpy(pserial, word, sizeof(word));

    *pserial = word[0];

    atecc508a_sleep();
    memcpy(buffer, serial_cache, sizeof(serial_cache));
    return result;
}

/**
 * @brief Reads either 4 or 32 bytes from the specified slot
 * @note  For 32 byte reads the offset is ignored.
 * @note  If SlotConfig.EncryptRead is set (encrypted read) then
 *        the GenDig command must have been run prior to this.
 * @param rw_ctx @ref ATMEL_RW_CTX_t
 * @return
 */
esp_err_t ATMEL_Read_Data(ATMEL_RW_CTX_t *const rw_ctx)
{
    uint16_t address = 0;
    esp_err_t result = ESP_OK;

    // 1. get the actual address for the chip to refer to
    ATMELUTIL_Get_Address(rw_ctx->zone, rw_ctx->slot, rw_ctx->block, rw_ctx->offset, &address);
    uint8_t param1 = rw_ctx->zone | ((32 == rw_ctx->len) ? 0x80 : 0x00);
    uint8_t arr_length = rw_ctx->len + 3;
    uint8_t response[arr_length];

    // 2. get the data from the chip
    result = atecc508a_response(response, arr_length, ATCA_READ, param1, address, NULL, 0, ATMEL_READ_DATA_DELAY_GENERIC);
    if (result != ESP_OK)
    {
        return result;
    }

    // 3. check if the response data length matches what is expected
    if (response[0] != arr_length)
    {
        return ESP_FAIL;
    }

    // 4. copy the data to the required buffer
    memcpy(rw_ctx->data, response + 1, rw_ctx->len);

    return result;
}

/**
 * @brief Calculates the the address based on the zone, slot, block and offset information.
 * @param zone
 * @param slot
 * @param block
 * @param offset
 * @param address
 */
void ATMELUTIL_Get_Address(const uint8_t zone, const uint8_t slot, const uint8_t block, uint8_t offset, uint16_t *const address)
{
    const uint8_t memzone = zone & 0x03;

    *address = 0;
    offset &= (uint8_t)0x07;

    if (memzone == 0x00 || memzone == 0x01)
    {
        *address = block << 3;
        *address |= offset;
    }
    else
    { /* ATCA_ZONE_DATA */
        *address = slot << 3;
        *address |= offset;
        *address |= block << 8;
    }
}

/**
 * @brief Write the configuration zone on the ATMEL508A chip
 *
 * @note The following bytes will be ignored since they are ready only.  The should
 * 		 be set to zero 0x00
 * 		 [0 - 15]
 * 		 [84 - 87]
 * 		 [88 - 91]
 *
 * @param[in] config The entire configuration zone data.  This must be 128 bytes
 * 					even though some data is read-only, these bytes will be ignored
 * 					so they should be set to 0x00
 *
 */
esp_err_t ATMELPROV_Set_Configuration_Zone(const uint8_t config[ATCA_CONFIG_SIZE])
{

    uint8_t len             = ATCA_CONFIG_SIZE;
    uint16_t write_index    = 0U;
    uint8_t current_block   = 0U;
    uint8_t current_offset  = 0U;
    uint8_t previous_block  = 0U;
    esp_err_t status        = ESP_OK;

    while (write_index < len)
    {
        // filter out read-only sections
        if (!(0U == current_block && (current_offset < 4U)) && !(2U == current_block && (5U == current_offset || 6U == current_offset)))
        {
            ATCAPACKET_t packet = {0};
            uint16_t address;
            do
            {
                Get_Address(ATCA_ZONE_CONFIG, 0U, current_block, current_offset, &address);
                memcpy(packet.data, &config[write_index], ATCA_WORD_SIZE);

                ESP_LOGI(TAG, "Data written at write_index %d: 0x%02X 0x%02X 0x%02X 0x%02X", write_index, packet.data[0], packet.data[1], packet.data[2], packet.data[3]);
                status = ATMEL_Write_Data(ATCA_ZONE_CONFIG, 0U, current_block, current_offset, &config[write_index], ATCA_WORD_SIZE);
                if (status != ESP_OK)
                {
                    ESP_LOGE(TAG, "Error writing the configs to chip");
                    break;
                }

            } while (0);
        }

        write_index += ATCA_WORD_SIZE;
        current_block = write_index / ATCA_BLOCK_SIZE;

        if (previous_block == current_block)
        {
            current_offset++;
        }
        else
        {
            current_offset = 0U;
            previous_block = current_block;
        }
    }

    return status;
}

/**
 * @brief Validates the writable bytes of the configuration zone match the supplied config
 *
 * @note The following bytes will not be checked since they are not writeable
 * 		 [0 - 15]
 * 		 [84 - 87]
 * 		 [88 - 91]
 *
 * @param[in] config The entire configuration zone data.  This must be 128 bytes
 * 					even though some data is read-only, these bytes will be ignored.
 *
 * @return CN_TRUE if the stored configuration matches the supplied configuration
 */
esp_err_t ATMELPROV_Validate_Configuration_Zone(const uint8_t config[ATCA_CONFIG_SIZE])
{
    uint8_t len             = ATCA_CONFIG_SIZE;
    uint16_t write_index    = 0U;
    uint8_t current_block   = 0U;
    uint8_t current_offset  = 0U;
    uint8_t previous_block  = 0U;
    esp_err_t status        = ESP_OK;
    uint8_t read_data[4];

    while (write_index < len)
    {
        // filter out read-only sections
        if (!(0U == current_block && (current_offset < 4U)) && !(2U == current_block && (5U == current_offset || 6U == current_offset)))
        {
            ATMEL_RW_CTX_t rw_ctx =
                {
                    .zone       = ATCA_ZONE_CONFIG,
                    .slot       = 0U,
                    .block      = current_block,
                    .offset     = current_offset,
                    .data       = read_data,
                    .len        = ATCA_WORD_SIZE,
                };

            if (ESP_OK != (status = ATMEL_Read_Data(&rw_ctx)))
            {
                ESP_LOGE(TAG, "error reading config from chip");
                break;
            }
            if (0 == memcmp(&config[write_index], read_data, ATCA_WORD_SIZE))
            {
                ESP_LOGW(TAG, "config data verification successful: read_data at write_index %d: 0x%02X 0x%02X 0x%02X 0x%02X", write_index, read_data[0], read_data[1], read_data[2], read_data[3]);
            }
            else
            {
                status = ESP_FAIL;
                break;
            }
        }

        write_index += ATCA_WORD_SIZE;
        current_block = write_index / ATCA_BLOCK_SIZE;

        if (previous_block == current_block)
        {
            current_offset++;
        }
        else
        {
            current_offset = 0U;
            previous_block = current_block;
        }
    }

    return status;
}

/**
 * @brief retrieves the slot locked status for each data slot.  Each slot represents one bit
 * 		  in the returned locked_status variable ( slot 0 = bit 0 ).  A zero in the slot bit
 * 		  means the slot is locked, and a 1 means its unlocked.
 *
 * @param locked_status[out] The slot locked status bits for each slot.
 * @return success of the operation
 */
esp_err_t ATMELPROV_Get_Data_Slot_Locked_Status(uint16_t *locked_status)
{
    esp_err_t status = ESP_OK;
    uint8_t data[4];
    ATMEL_RW_CTX_t rw_ctx =
        {
            .zone       = ATCA_ZONE_CONFIG,
            .slot       = 0U,
            .block      = 2U,
            .offset     = 6U,
            .data       = data,
            .len        = 4U,
        };

    status = ATMEL_Read_Data(&rw_ctx);
    *locked_status = (data[1] << 8) | data[0];

    return status;
}

/**
 * @brief Checks if a particular zone is locked or not
 *
 * @note The device is put to sleep via #ATMEL_Sleep after executing this command.
 *
 * @param[in]  zone				the zone to check either
 * 								#ATCA_ZONE_DATA or #ATCA_ZONE_CONFIG or ATCA_ZONE_OTP
 * @param[out] locked_status		if the operation status is #ATCA_SUCCESS then this identifies if
 * 								the supplied zone is locked (CN_TRUE) or unlocked (CN_FALSE).
 * 								If the operation failed then this value is irrelevant
 *
 * @return	Status of the operation
 */
esp_err_t ATMEL_Is_Zone_Locked(const uint8_t zone, bool *locked_status)
{
    esp_err_t status = ESP_OK;
    uint8_t byte;
    uint8_t data[4U];

    if (ATCA_ZONE_DATA == zone || ATCA_ZONE_OTP == zone)
    {
        byte = 86;
    }
    else if (ATCA_ZONE_CONFIG == zone)
    {
        byte = 87U;
    }
    else
    {
        return ESP_FAIL;
    }

    const uint8_t block         = byte / ATCA_BLOCK_SIZE;
    const uint8_t offset        = (byte - (block * ATCA_BLOCK_SIZE) - 1) / ATCA_WORD_SIZE;
    const uint8_t word_index    = (byte - (block * ATCA_BLOCK_SIZE) - (offset * ATCA_WORD_SIZE));

    ATMEL_RW_CTX_t rw_ctx =
        {
            .zone       = ATCA_ZONE_CONFIG,
            .slot       = 0U,
            .block      = block,
            .offset     = offset,
            .data       = data,
            .len        = 4U,
        };

    status = ATMEL_Read_Data(&rw_ctx);
    atecc508a_sleep();

    if (ESP_OK == status)
    {
        if (0x55 == *(data + word_index))
        {
            *locked_status = false;
        }
        else if (0x00 == *(data + word_index))
        {
            *locked_status = true;
        }
        else
        {
            // error invalid value
            *locked_status = false;
            status         = ESP_FAIL;
        }
    }

    return status;
}

/**
 * @brief Sends a lock zone command
 *
 * @param lock_zone[in] either LOCK_ZONE_CONFIG or LOCK_ZONE_DATA
 * @param slot[in]		If supplied then the data slot will be locked rather than a whole zone.
 * 						Note: You must supply #LOCK_ZONE_DATA_SLOT for lock_zone if you
 * 						want to lock a data slot.  If locking a zone then supply 0.
 * @param crc[in]		The 2 byte CRC for the zone or slot to be locked. if CN_NULL then
 * 						CRC won't be validated
 *
 * @return status of operation
 */
esp_err_t ATMELPROV_Lock(uint8_t lock_zone, uint8_t slot, const uint8_t *crc)
{
    esp_err_t status = ESP_OK;
    uint8_t response[LOCK_RSP_SIZE];
    ATCAPACKET_t packet = {0};
    packet.word_address = 0x03;
    packet.param1 = lock_zone | (slot << 2);
    if (NULL != crc)
    {
        packet.param2 = (crc[1] << 8) | crc[0];
    }
    else
    {
        packet.param2 = 0U;
        packet.param1 |= LOCK_ZONE_NO_CRC;
    }
    packet.opcode = ATCA_LOCK;
    packet.txsize = LOCK_COUNT;
    packet.rxsize = LOCK_RSP_SIZE;

    status = atecc508a_response(response, LOCK_RSP_SIZE, ATCA_LOCK, packet.param1, packet.param2, NULL, 0, ATMEL_READ_DATA_DELAY_GENERIC);
    
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "sending lock command failed");
    }
    return status;
}

/**
 * @brief Calculates the code authorization session key based on the input parameters
 *
 * @note The supplied nonce should have come from a previous call to the #ATMEL_Nonce_Random function
 *
 * @param[in]  zone					The zone which was used on the #ATEML_Gen_Digest call
 * @param[in]  nonce					The nonce used in the #ATMEL_Gen_Digest call
 * @param[out] session_key			The calculated session key.
 * @return status of the operation
 */
static esp_err_t Calculate_Session_Key(const uint8_t zone, const uint8_t *nonce, uint8_t *session_key)
{
    struct SHA256_CTX_t ctx;
    const uint8_t opt_code = ATCA_GENDIG;
    uint8_t code_auth_key[ATMEL_KEY_SIZE];
    uint8_t serial[9];
    uint8_t zeros[25];
    memset(zeros, 0x00, 25U);

    if (ESP_OK != ATMEL_Get_Serial_ID(serial))
    {
        return ESP_FAIL;
    }

    // Calculate Session Key
    ATMELPLA_Get_Code_Authorization(code_auth_key);
    CENSE_Sha256_Start(&ctx);
    CENSE_Sha256_Update(&ctx, code_auth_key, ATMEL_KEY_SIZE);
    CENSE_Sha256_Update(&ctx, &opt_code, 1U);
    CENSE_Sha256_Update(&ctx, &zone, 1U);
    CENSE_Sha256_Update(&ctx, &code_auth_key_slot, 1U);
    CENSE_Sha256_Update(&ctx, zeros, 1U);
    CENSE_Sha256_Update(&ctx, &serial[8], 1U);
    CENSE_Sha256_Update(&ctx, serial, 2U);
    CENSE_Sha256_Update(&ctx, zeros, 25U);
    CENSE_Sha256_Update(&ctx, nonce, ATMEL_NONCE_SIZE);
    CENSE_Sha256_Final(&ctx, session_key);

    return ESP_OK;
}

/**
 * @brief Performs a 32-byte encrypted write to a zone on the ATMEL508A chip
 *
 * @note The device is put to sleep via #ATMEL_Sleep after executing this command.
 *
 * @param[in]  zone		The zone to perform the write on
 * @param[in]  slot		The slot to perform the write on
 * @param[in]  block		The block to perform the write on
 * @param[in]  data		32 bytes of plain text data to write
 *
 * @return status of operation
 */
esp_err_t ATMEL_Encrypted_Write(const uint8_t zone, const uint8_t slot, const uint8_t block, const uint8_t *data)
{
    /*
     * Encrypted Writes are done by performing the following
     * 1. Generate Random Nonce
     * 2. Perform Gen Digest on Read Key ID to generate Session Key on ATMEL508A
     * 3. Calculate Nonce by doing Software SHA 256 on Random, Salt, Nonce OPT code (0x16), Mode (0x00), 0x00
     * 4. Calculate Session Key by doing software SHA 256 on Read Key, GenDigest OPT code (0x15),
     * 	  Zone, Read Key ID, SN[8], SN[0:1], 25 Zero bytes, Nonce
     * 5. Write Data
     * 6. Software Decrypt with Session Key
     */

    esp_err_t status = ESP_FAIL;
    uint8_t nonce[ATMEL_NONCE_SIZE];
    uint8_t session_key[ATMEL_KEY_SIZE];
    uint8_t salt[ATMEL_SALT_SIZE];
    uint8_t serial[ATMEL_SERIAL_LENGTH];

    ATMELPLA_Generate_Salt(salt);

    do
    {
        if (ESP_FAIL == ATMEL_Get_Serial_ID(serial))
        {
            status = ESP_FAIL;
            break;
        }
        if (ESP_OK != (status = ATMEL_Nonce_Random(salt, nonce)))
        {
            break;
        }

        if (ESP_OK != (status = ATMEL_Gen_Digest(ATCA_ZONE_DATA, code_auth_key_slot)))
        {
            break;
        }

        if (ESP_OK != (status = Calculate_Session_Key(ATCA_ZONE_DATA, nonce, session_key)))
        {
            break;
        }
        ATCAPACKET_t packet = {0};
        uint8_t response[WRITE_RSP_SIZE] = {0};
        uint16_t address;

        // encrypt data
        uint8_t *enc_data = packet.data;
        memcpy(enc_data, data, ATMEL_BLOCK_SIZE);

        // encrypt data
        for (uint32_t i = ATMEL_BLOCK_SIZE; i--;)
        {
            enc_data[i] ^= session_key[i];
        }

        Get_Address(zone, slot, block, 0U, &address);

        uint8_t param2_bytes[2] = {(address & 0xFF), ((address >> 8) & 0xFF)};
        packet.word_address     = 0x03;
        packet.param1           = zone | ATCA_ZONE_READWRITE_32;
        packet.param2           = address;
        packet.opcode           = ATCA_WRITE;
        packet.txsize           = WRITE_COUNT_LONG_MAC;
        packet.rxsize           = WRITE_RSP_SIZE;

        // create validation MAC to validate write
        struct SHA256_CTX_t ctx;
        CENSE_Sha256_Start(&ctx);
        CENSE_Sha256_Update(&ctx, session_key, ATMEL_KEY_SIZE);
        CENSE_Sha256_Update(&ctx, &packet.opcode, 1U);
        CENSE_Sha256_Update(&ctx, &packet.param1, 1U);
        CENSE_Sha256_Update(&ctx, param2_bytes, 2U);
        CENSE_Sha256_Update(&ctx, &serial[8], 1U);
        CENSE_Sha256_Update(&ctx, serial, 2U);
        CENSE_Sha256_Update_Zeros(&ctx, 25U);
        CENSE_Sha256_Update(&ctx, data, ATMEL_BLOCK_SIZE);
        CENSE_Sha256_Final(&ctx, &packet.data[ATMEL_BLOCK_SIZE]);

        status = atecc508a_response(response, WRITE_RSP_SIZE, packet.opcode, packet.param1, packet.param2, packet.data, packet.txsize, ATMEL_READ_DATA_DELAY_GENERIC);
        if (status == ESP_OK)
        {
            if (response[1] != ATECA_RESPONSE_SUCCESS)
            {
                status = ESP_FAIL;
            }
        }
    } while (0);

    atecc508a_sleep();
    return status;
}

/**
 * @brief Executes the Gen Digest command on the ATMEL508A to perform a SHA 256 calculation
 * 		  by combining the supplied key with the value in tempKey.
 *
 * 		  If this call is being used for an encrypted read then the key_slot should be the
 * 		  slot of the Code Authorization key
 *
 * @param[in] zone		The zone to perform that digest on
 * @param[in] key_slot	For the data zone, this is the key slot to perform the digest on.
 * 						For the config zone or OTP zone, this specifies the block number
 * 						For the OTP zone, this specifies the
 *
 * @return status of operation
 */
esp_err_t ATMEL_Gen_Digest(const uint8_t zone, const uint8_t key_slot)
{
    uint8_t response[GENKEY_RSP_SIZE_SHORT] = {0};
    ATCAPACKET_t packet     = {0};
    packet.word_address     = 0x03;
    packet.opcode           = ATCA_GENDIG;
    packet.param1           = zone;
    packet.param2           = key_slot;
    packet.txsize           = GENKEY_COUNT;
    packet.rxsize           = GENKEY_RSP_SIZE_SHORT;

    esp_err_t status = atecc508a_response(response, GENKEY_RSP_SIZE_SHORT, packet.opcode, packet.param1, packet.param2, NULL, 0, ATMEL_READ_DATA_DELAY_GENKEY);
    if (status == ESP_OK)
    {
        if (response[1] != ATECA_RESPONSE_SUCCESS)
        {
            ESP_LOGE(TAG, "Error executing Gen_Digest command");
            status = ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error getting response for GEN_Digest");
    }

    return status;
}



esp_err_t ATMEL_Write_Data(const uint8_t zone, const uint8_t slot, const uint8_t block, const uint8_t offset, const uint8_t *data, const uint8_t len)
{
    esp_err_t status    = ESP_OK;
    ATCAPACKET_t packet = {0};
    uint16_t address;
    uint8_t response[WRITE_RSP_SIZE] = {0};

    do
    {
        Get_Address(zone, slot, block, offset, &address);
        packet.param1 = zone;
        packet.param2 = address;
        if (len == ATCA_BLOCK_SIZE)
        {
            packet.param1 |= ATCA_ZONE_READWRITE_32;
        }
        packet.opcode = ATCA_WRITE;
        packet.txsize = (len == ATCA_WORD_SIZE ? WRITE_COUNT_SHORT : WRITE_COUNT_LONG);
        memcpy(packet.data, data, len);

        status = atecc508a_response(response, WRITE_RSP_SIZE, packet.opcode, packet.param1, packet.param2, packet.data, packet.txsize, ATMEL_READ_DATA_DELAY_GENERIC);

        if (status == ESP_OK)
        {
            if (response[1] != ATECA_RESPONSE_SUCCESS)
            {
                status = ESP_FAIL;
            }
        }

    } while (0);

    return status;
}

/**
 * @brief This function behaves differently depending on the specified mode.
 *
 * 		  Mode #GENKEY_MODE_PRIVATE_KEY_GENERATE
 * 		  	Generates a new private key in the key_slot specified and returns the public key.
 * 		  	If the data zone is locked and the key_slot is defined to require authorization
 * 		  	the the requires_authorization flag must be CN_TRUE or else it should be CN_FALSE.
 *
 *		  Mode #GENKEY_MODE_PUBLIC_KEY_DIGEST
 *		  	In this mode The device creates a public key digest based on the private key in the key_slot
 *		  	and places it in tempKey
 *
 *		  Mode #GENKEY_MODE_DIGEST_IN_TEMPKEY
 *		  	In this mode key_slot must point to a public key and this command only creates the digest
 *		  	and stores it in tempKey (without creating the public key)
 *
 * 		  Once a private key is generated there is no way to read the private key.
 *
 * @note The device is put to sleep via #ATMEL_Sleep after executing this command unless the mode is
 * 		 GENKEY_MODE_DIGEST_IN_TEMPKEY in which case it is not put to sleep
 *
 * @param[in]  key_slot					Depending on the mode this is either the private key slot to write to or
 * 										the key slot of a public key to create a digest from.
 * @param[in]  requires_authorization	If authorization is required by the key slot (only
 * 										applicable if data zone is locked)
 * @param[out] public_key				in Mode #GENKEY_MODE_PRIVATE_KEY_GENERATE this buffer will be used to
 * 										store the 64 byte public key corresponding to the private key generated.
 * 										in all other modes this variable can be CN_NULL as it is not used.
 *
 * @return success of the operation
 */
esp_err_t ATMEL_Gen_Key(const uint8_t key_slot, const uint8_t mode, const bool requires_authorization, uint8_t *public_key)
{
    esp_err_t status = ESP_FAIL;
    do
    {
        if (requires_authorization)
        {
            if (ESP_OK != (status = Authorize_MAC()))
            {
                break;
            }
        }

        uint8_t response[GENKEY_RSP_SIZE_MID] = {0};
        ATCAPACKET_t packet     = {0};
        packet.word_address     = 0x03;
        packet.opcode           = ATCA_GENKEY;
        packet.param1           = mode;
        packet.param2           = key_slot;
        packet.txsize           = GENKEY_COUNT;
        packet.rxsize           = (GENKEY_MODE_PRIVATE_KEY_GENERATE == mode) ? 67U : GENKEY_RSP_SIZE_SHORT;

        status = atecc508a_response(response, GENKEY_RSP_SIZE_MID, packet.opcode, packet.param1, packet.param2, NULL, 0, ATMEL_READ_DATA_DELAY_GENKEY);

        if (status != ESP_OK)
        {
            atecc508a_sleep();
            break;
        }
        if (GENKEY_MODE_DIGEST_IN_TEMPKEY != mode)
        {
            atecc508a_sleep();
        }
        if (GENKEY_MODE_PRIVATE_KEY_GENERATE == mode && NULL != public_key)
        {
            const uint8_t *sourcePtr = response + 1;
            memcpy(public_key, sourcePtr, ATMEL_PUBLIC_KEY_SIZE);
        }
    } while (0);

    return status;
}

/**
 * @brief Returns the code authorization key used for encrypted reads
 *
 * @param key[out] code authorization key will be stored here.
 */
void ATMELPLA_Get_Code_Authorization(uint8_t *key)
{
    const uint8_t data_2[36] = {0x69, 0xf6, 0x0f, 0x46, 0xd0, 0xf4, 0x02, 0xb7, 0x8e, 0x25, 0x2c, 0x20, 0x94, 0x3c, 0xff, 0xac, 0x1f, 0x03, 0x73, 0x7b, 0x99, 0x43, 0x2f, 0xa5, 0x84, 0x68, 0xbd, 0x68, 0x64, 0xa4, 0x1f, 0x1f};

    for (uint32_t i = 32; i--;)
    {
        *(key + i) = data_2[i] ^ 0xA3;
    }
}

#endif