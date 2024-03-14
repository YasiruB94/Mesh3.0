#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_HANDSHAKE_H
#define CNGW_HANDSHAKE_H

#include "cngw_common.h"

typedef enum CNGW_Handshake_Command
{
	CNGW_Handshake_CMD_CN1 = 0x01,
	CNGW_Handshake_CMD_GW1 = 0x04,
	CNGW_Handshake_CMD_CN2 = 0x02,
	CNGW_Handshake_CMD_GW2 = 0x03,
} __attribute__((packed)) CNGW_Handshake_Command;

typedef enum CNGW_Handshake_Status
{
	CNGW_Handshake_STATUS_SUCCESS = 0x01,
	CNGW_Handshake_STATUS_FAILED = 0x02
} __attribute__((packed)) CNGW_Handshake_Status;

/**
 * @brief Structure for the initial handshake CN1 message between Control MCU and Gateway MCU
 *
 * The #CNGW_Handshake_CMD_CN1 is the beginning message to the handshake process.  It includes
 * the serial number of the mainboard as well as a challenge to be used in a
 * challenge / response process
 *
 * Message is 94 bytes in length (not including header)
 *
 * @note This is send under the header type #CNGW_HEADER_TYPE_Handshake_Command
 */
typedef struct CNGW_Handshake_CN1_t
{
	CNGW_Handshake_Command command;
	uint8_t mainboard_serial[CNGW_SERIAL_NUMBER_LENGTH]; /** @brief the Serial number of the CENSE mainboard */
	uint16_t cabinet_number;							 /** @brief The cabinet number as indicated by the CN MCU*/
	uint8_t challenge[CNGW_CHALLENGE_RESPONSE_LENGTH];	 /** @brief the challenge/response challenge from the mainboard */
	uint8_t hmac[CNGW_HMAC_LENGTH];						 /** @brief HMAC of this entire structure (excluding the HMAC itself */

} __attribute__((packed)) CNGW_Handshake_CN1_t;

/**
 * @brief A generic ACK message sent from the MCU as the last
 *        message from CN or GW
 */
typedef struct CNGW_Handshake_Ack_t
{
	CNGW_Handshake_Command command; /** @brief Must CNGW_Handshake_CMD_xx2. Depending on if originating from CN or GW*/
	CNGW_Handshake_Status status;	/** @brief The handshake status depending on fail or pass*/
	uint8_t hmac[CNGW_HMAC_LENGTH]; /** @brief HMAC of this entire structure (excluding the HMAC itself */
} __attribute__((packed)) CNGW_Handshake_Ack_t;

/**
 * @brief The ACK message for CN2 sent as
 *        CNGW_HEADER_TYPE_Handshake_Command
 */
typedef CNGW_Handshake_Ack_t CNGW_Handshake_CN2_t;

/**
 * @brief Memory space for the largest possible CN message
 */
typedef union CCP_HANDSHAKE_CN_MESSAGES_t
{
	/**@brief The command field for the messages below.
	 * This command field is always the first byte in each
	 * message*/
	CNGW_Handshake_Command command;

	CNGW_Handshake_CN1_t cn1_message;
	CNGW_Handshake_CN2_t cn2_message;
} __attribute__((packed)) CCP_HANDSHAKE_CN_MESSAGES_t;

/**
 * @brief Structure for the handshake GW1 response message from Gateway to Control MCU
 *
 * This message includes information about the gateway (serial number, firmware version, etc) as well as the
 * response to the challenge which was received in the #CNGW_Handshake_CMD_CN1.  THe response should be generated using
 * the ATMEL chip's #ATMEL_Challenge_MAC function
 *
 * Message is 93 bytes in length (not including header)
 *
 * @note This is send under the header type #CNGW_HEADER_TYPE_Handshake_Response
 */
typedef struct CNGW_Handshake_GW1_t
{
	CNGW_Handshake_Command command;								/** @brief Handshake command number. Should be #CNGW_Handshake_CMD_GW1 */
	CNGW_Handshake_Status status;								/** @brief handshaking status (status of processing the CN1/GW1 message */
	uint8_t gateway_serial[CNGW_SERIAL_NUMBER_LENGTH];			/** @brief ATMEL Serial Number of the gateway */
	uint16_t gateway_model;										/** @brief model number of the gateway */
	CNGW_Firmware_Version_t firmware_version;					/** @brief The gateway firmware version*/
	CNGW_Firmware_Version_t bootloader_version;					/** @brief The gateway bootloader version*/
	uint32_t reserved;											/** @brief Not used, can be left 0x00000000 */
	uint8_t challenge_response[CNGW_CHALLENGE_RESPONSE_LENGTH]; /** @brief Response to the challenge sent in the CN1 message */
	uint8_t hmac[CNGW_HMAC_LENGTH];								/** @brief HMAC of this entire structure (excluding the HMAC itself */
} __attribute__((packed)) CNGW_Handshake_GW1_t;

/**
 * @brief The frame for CNGW_Handshake_CN1_t
 */
typedef struct CNGW_Handshake_CN1_Frame_t
{
	CNGW_Message_Header_t header;
	CNGW_Handshake_CN1_t message;
} __attribute__((packed)) CNGW_Handshake_CN1_Frame_t;

/**
 * @brief The frame for CNGW_Handshake_Ack_t
 */
typedef struct CNGW_Handshake_Ack_Frame_t
{
	CNGW_Message_Header_t header;
	CNGW_Handshake_Ack_t message;
} __attribute__((packed)) CNGW_Handshake_Ack_Frame_t;

typedef CNGW_Handshake_Ack_Frame_t CNGW_Handshake_CN2_Frame_t;

typedef CNGW_Handshake_Ack_Frame_t CNGW_Handshake_GW2_Frame_t;

/**
 * @brief The frame for CNGW_Handshake_GW1_t
 */
typedef struct CNGW_Handshake_GW1_Frame_t
{
	CNGW_Message_Header_t header;
	CNGW_Handshake_GW1_t message;
} __attribute__((packed)) CNGW_Handshake_GW1_Frame_t;

#endif
#endif