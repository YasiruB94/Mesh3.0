/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: crypto.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used to provision a crypto chip, all crypto related
 * functions
 ******************************************************************************
 *
 ******************************************************************************
 */


#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CRYPTO_H
#define CRYPTO_H
#include "includes/SpacrGateway_commands.h"
#include "at508_structs.h"
#include "gw_includes/crypto/gw_keys_bb.h"

uint8_t unsuccessful_handshake_attempts;

const uint8_t *OS_Get_512_Zeros(void);
void SHA256_Zero(mbedtls_sha256_context *ctx, const uint16_t count);
void ATMELUTIL_Get_Code_Authorization(uint8_t *const key);
void ATMELUTIL_Get_Address(const uint8_t zone, const uint8_t slot, const uint8_t block, uint8_t offset, uint16_t *const address);
void ATMELPLA_Get_Code_Authorization(uint8_t * key);
void ATMEL_Add_CRC(const uint8_t length, const uint8_t *data, uint8_t *crc);
void ATMEL_Calculate_CRC(ATCAPACKET_t * packet);
esp_err_t Authorize_MAC();
esp_err_t ATMEL_Validate_HMAC(const uint8_t *message, const uint16_t length, const uint8_t key_slot, const uint8_t *hmac);
esp_err_t ATMEL_Nonce_Passthrough(uint8_t * tempkey);
esp_err_t ATMEL_Nonce_Random(const uint8_t *const salt, uint8_t *const nonce);
esp_err_t ATMEL_Get_Serial_ID(uint8_t *const buffer);
esp_err_t ATMEL_Read_Data(ATMEL_RW_CTX_t *const rw_ctx);
esp_err_t ATMEL_HMAC(const uint8_t *const message, const uint32_t length, const uint8_t key_slot, uint8_t *hmac);
esp_err_t ATMEL_Challenge_MAC(uint8_t * challenge, const uint8_t device_auth_key, uint8_t *const response);
esp_err_t ATMELPROV_Set_Configuration_Zone(const uint8_t config[ATCA_CONFIG_SIZE]);
esp_err_t ATMELPROV_Validate_Configuration_Zone(const uint8_t config[ATCA_CONFIG_SIZE]);
esp_err_t ATMELPROV_Get_Data_Slot_Locked_Status(uint16_t *locked_status);
esp_err_t ATMEL_Is_Zone_Locked(const uint8_t zone, bool * locked_status);
esp_err_t ATMELPROV_Lock(uint8_t lock_zone, uint8_t slot, const uint8_t * crc);
esp_err_t ATMEL_Encrypted_Write(const uint8_t zone, const uint8_t slot, const uint8_t block, const uint8_t * data);
esp_err_t ATMEL_Write_Data(const uint8_t zone, const uint8_t slot, const uint8_t block, const uint8_t offset, const uint8_t *data, const uint8_t len);
esp_err_t ATMEL_Gen_Key(const uint8_t key_slot, const uint8_t mode, const bool requires_authorization, uint8_t * public_key);
esp_err_t ATMEL_Gen_Digest(const uint8_t zone, const uint8_t key_slot);

#endif
#endif