/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: atecc508a.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used by the crypto chip to get a response and to generate
 * a SALT
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef ATECC508A_H
#define ATECC508A_H
#include "atecc508a_comm.h"

#define CENSE_PRODUCT_ID_CABINET    (0x0001)
#define CENSE_PRODUCT_ID_LM1        (0x0002)
#define CENSE_MODEL_NUMBER_CABINET  (0x0001)
#define CENSE_MODEL_NUMBER_LM1      (0x0010)
#define LM1_CHANNEL_COUNT 4U

#define ATECC508A_CRC_POLYNOM           0x8005
#define ATECC508A_ADDR                  0x60
#define ATMEL_READ_DATA_DELAY_GENERIC   ((uint8_t)50)
#define ATMEL_READ_DATA_DELAY_HMAC      ((uint8_t)50)
#define ATMEL_READ_DATA_DELAY_GENKEY    ((uint8_t)150)
#define ATMEL_RESPONSE_TIMEOUT_GENERIC  10
#define ATMEL_RESPONSE_TIMEOUT_SLEEP    10
#define ACK_CHECK_EN                    0x1                     /*!< I2C master will check ack from slave */
#define ACK_CHECK_DIS                   0x0                     /*!< I2C master will not check ack from slave */

#define ATCA_CONFIG_SIZE                (128)                   //!< size of configuration zone
#define ATCA_CHECKMAC                   ((uint8_t)0x28)         //!< CheckMac command op-code
#define ATCA_DERIVE_KEY                 ((uint8_t)0x1C)         //!< DeriveKey command op-code
#define ATCA_INFO                       ((uint8_t)0x30)         //!< Info command op-code
#define ATCA_GENDIG                     ((uint8_t)0x15)         //!< GenDig command op-code
#define ATCA_GENKEY                     ((uint8_t)0x40)         //!< GenKey command op-code
#define ATCA_HMAC                       ((uint8_t)0x11)         //!< HMAC command op-code
#define ATCA_LOCK                       ((uint8_t)0x17)         //!< Lock command op-code
#define ATCA_MAC                        ((uint8_t)0x08)         //!< MAC command op-code
#define ATCA_NONCE                      ((uint8_t)0x16)         //!< Nonce command op-code
#define ATCA_PAUSE                      ((uint8_t)0x01)         //!< Pause command op-code
#define ATCA_PRIVWRITE                  ((uint8_t)0x46)         //!< PrivWrite command op-code
#define ATCA_RANDOM                     ((uint8_t)0x1B)         //!< Random command op-code
#define ATCA_READ                       ((uint8_t)0x02)         //!< Read command op-code
#define ATCA_SIGN                       ((uint8_t)0x41)         //!< Sign command op-code
#define ATCA_UPDATE_EXTRA               ((uint8_t)0x20)         //!< UpdateExtra command op-code
#define ATCA_VERIFY                     ((uint8_t)0x45)         //!< GenKey command op-code
#define ATCA_WRITE                      ((uint8_t)0x12)         //!< Write command op-code
#define ATCA_ECDH                       ((uint8_t)0x43)         //!< ECDH command op-code
#define ATCA_COUNTER                    ((uint8_t)0x24)         //!< Counter command op-code
#define ATCA_SHA                        ((uint8_t)0x47)         //!< SHA command op-code

#define ATECA_RESPONSE_WAKE             ((uint8_t)0x11)         //!< Response for a successful wake up, prior to first command
#define ATECA_RESPONSE_SUCCESS          ((uint8_t)0x00)         //!< Response for a successful command execution
#define ATECA_RESPONSE_MISCOMPARE       ((uint8_t)0x01)         //!< Response for a checkmac or verification miscompare
#define ATECA_RESPONSE_PARSE_ERROR      ((uint8_t)0x03)         //!< Response for a parse error
#define ATECA_RESPONSE_ECC_FAULT        ((uint8_t)0x05)         //!< Response for a ECC fault
#define ATECA_RESPONSE_EXEC_ERROR       ((uint8_t)0x0F)         //!< Response for a execution error
#define ATECA_RESPONSE_WATCHDOG_EXPIRE  ((uint8_t)0xEE)         //!< Response for a watchdog about to expire
#define ATECA_RESPONSE_ERROR            ((uint8_t)0xFF)         //!< Response for a CRC or other communication error

#define WRITE_COUNT_SHORT              (11)                    //!< Write command packet size with short data and no MAC
#define ATCA_BLOCK_SIZE                (32)                    //!< size of a block
#define ATCA_WORD_SIZE              	(4)                     //!< size of a word
#define ATCA_CMD_SIZE_MIN       		   ((uint8_t)7)
#define ATCA_RSP_SIZE_MIN           	((uint8_t)4)            //!< minimum number of bytes in response
#define ATCA_RSP_SIZE_4             	((uint8_t)7)            //!< size of response packet containing 4 bytes data
#define ATCA_RSP_SIZE_72            	((uint8_t)75)           //!< size of response packet containing 64 bytes data
#define ATCA_RSP_SIZE_64            	((uint8_t)67)           //!< size of response packet containing 64 bytes data
#define ATCA_RSP_SIZE_32            	((uint8_t)35)           //!< size of response packet containing 32 bytes data
#define ATCA_RSP_SIZE_MAX           	((uint8_t)75)           //!< maximum size of response packet (GenKey and Verify command)
#define ATCA_CRC_SIZE               	((uint8_t)2)            //!< Number of bytes in the command packet CRC
#define RANDOM_COUNT                   ATCA_CMD_SIZE_MIN       //!< Random command packet size
#define RANDOM_RSP_SIZE             	ATCA_RSP_SIZE_32        //!< response size of Random command
#define WRITE_RSP_SIZE                 ATCA_RSP_SIZE_MIN       //!< response size of Write command
#define WRITE_COUNT_SHORT              (11)                    //!< Write command packet size with short data and no MAC
#define WRITE_COUNT_LONG               (39)                    //!< Write command packet size with long data and no MAC
#define WRITE_COUNT_LONG_MAC           (71)                //!< Write command packet size with long data and MAC


#define ATMEL_SERIAL_LENGTH             (9u)
#define ATMEL_ZONE_CONFIGURATION        (0x00)
#define ATMEL_ZOME_DATA                 (0x01)
#define ATMEL_ZONE_SINGLE_SLOT          (0x02)
#define ATMEL_WORD_SIZE                 (4u)
#define ATMEL_BLOCK_SIZE                (32u)
#define ATMEL_SHA_BLOCK_SIZE            (64u)
#define ATMEL_HASH_SIZE                 (32u)
#define ATMEL_HMAC_SIZE                 (32u)
#define ATMEL_MAC_SIZE                  (32u)
#define ATMEL_RANDOM_SIZE               (32u)
#define ATMEL_NONCE_SIZE                (32u)
#define ATMEL_SIGNATURE_SIZE            (64u)
#define ATMEL_WORD_ADDRESS_COMMAND      (0x03)
#define ATMEL_KEY_SIZE                  (32u)
#define ATMEL_PUBLIC_KEY_SIZE           (64u)
#define ATMEL_SALT_SIZE                 (20u)
#define CHECKMAC_CLIENT_CHALLENGE_SIZE  (32)                    //!< CheckMAC size of client challenge
#define CHECKMAC_CLIENT_RESPONSE_SIZE   (32)                    //!< CheckMAC size of client response
#define CHECKMAC_OTHER_DATA_SIZE        (13)                    //!< CheckMAC size of "other data"

/** \name Definitions for Zone and Address Parameters
   @{ */
#define ATCA_ZONE_CONFIG                ((uint8_t)0x00)         //!< Configuration zone
#define ATCA_ZONE_OTP                   ((uint8_t)0x01)         //!< OTP (One Time Programming) zone
#define ATCA_ZONE_DATA                  ((uint8_t)0x02)         //!< Data zone
#define ATCA_ZONE_MASK                  ((uint8_t)0x03)         //!< Zone mask
#define ATCA_ZONE_READWRITE_32          ((uint8_t)0x80)         //!< Zone bit 7 set: Access 32 bytes, otherwise 4 bytes.
#define ATCA_ZONE_ACCESS_4              ((uint8_t)4)            //!< Read or write 4 bytes.
#define ATCA_ZONE_ACCESS_32             ((uint8_t)32)           //!< Read or write 32 bytes.
#define ATCA_ADDRESS_MASK_CONFIG        (0x001F)                //!< Address bits 5 to 7 are 0 for Configuration zone.
#define ATCA_ADDRESS_MASK_OTP           (0x000F)                //!< Address bits 4 to 7 are 0 for OTP zone.
#define ATCA_ADDRESS_MASK               (0x007F)                //!< Address bit 7 to 15 are always 0.
/** @} */
/** \name Definitions for the Lock Command
   @{ */
#define LOCK_ZONE_IDX               ATCA_PARAM1_IDX     //!< Lock command index for zone
#define LOCK_SUMMARY_IDX            ATCA_PARAM2_IDX     //!< Lock command index for summary
#define LOCK_COUNT                  ATCA_CMD_SIZE_MIN   //!< Lock command packet size
#define LOCK_ZONE_CONFIG            ((uint8_t)0x00)     //!< Lock zone is Config
#define LOCK_ZONE_DATA              ((uint8_t)0x01)     //!< Lock zone is OTP or Data
#define LOCK_ZONE_DATA_SLOT         ((uint8_t)0x02)     //!< Lock slot of Data
#define LOCK_ZONE_NO_CRC            ((uint8_t)0x80)     //!< Lock command: Ignore summary.
#define LOCK_ZONE_MASK              (0xBF)              //!< Lock parameter 1 bits 6 are 0.
/** @} */

#define LOCK_RSP_SIZE               ATCA_RSP_SIZE_MIN   //!< response size of Lock command

/** @} */

/** \name Definitions for the GenKey Command
   @{ */
#define GENKEY_MODE_IDX             ATCA_PARAM1_IDX         //!< GenKey command index for mode
#define GENKEY_KEYID_IDX            ATCA_PARAM2_IDX         //!< GenKey command index for key id
#define GENKEY_DATA_IDX             (5)                     //!< GenKey command index for other data
#define GENKEY_COUNT                ATCA_CMD_SIZE_MIN       //!< GenKey command packet size without "other data"
#define GENKEY_COUNT_DATA           (10)                    //!< GenKey command packet size with "other data"
#define GENKEY_OTHER_DATA_SIZE      (3)                     //!< GenKey size of "other data"
#define GENKEY_MODE_MASK            ((uint8_t)0x1C)         //!< GenKey mode bits 0 to 1 and 5 to 7 are 0
#define GENKEY_MODE_PRIVATE         ((uint8_t)0x04)         //!< GenKey mode: private key generation
#define GENKEY_MODE_PUBLIC          ((uint8_t)0x00)         //!< GenKey mode: public key calculation
#define GENKEY_MODE_DIGEST          ((uint8_t)0x10)         //!< GenKey mode: digest calculation
#define GENKEY_MODE_ADD_DIGEST      ((uint8_t)0x08)         //!< GenKey mode: additional digest calculation
#define GENKEY_RSP_SIZE_SHORT       ATCA_RSP_SIZE_MIN       //!< response size of GenKey command in Digest mode
#define GENKEY_RSP_SIZE_LONG        ATCA_RSP_SIZE_72        //!< response size of GenKey command when generating key
#define GENKEY_RSP_SIZE_MID         ATCA_RSP_SIZE_64        //!< response size of GenKey command when generating key

typedef enum 
{
	GENKEY_MODE_PRIVATE_KEY_GENERATE = ((uint8_t) 0x04),     //!< Private Key Creation
	GENKEY_MODE_PUBLIC_KEY_DIGEST    = ((uint8_t) 0x08),     //!< Public Key Computation
	GENKEY_MODE_DIGEST_IN_TEMPKEY    = ((uint8_t) 0x10)      //!< Digest Calculation
} enum_genkey_mode;


esp_err_t atecc508a_response(uint8_t *response, uint8_t length, uint8_t command, uint8_t param1, uint16_t param2, uint8_t *data, size_t ota_agent_core_data_length, uint8_t delay_between_send_and_recv);
void ATMELPLA_Generate_Salt(uint8_t *salt);
#endif
#endif