#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CNGW_OTA_H
#define CNGW_OTA_H

#include "cngw_common.h"
#include "freertos/task.h"

#define CNGW_MAX_OTA_DONWLOAD_TIME_MS (20u * 1000u)
#define OTA_DEFAULT_SESSION_TIMEOUT_TICKS (pdMS_TO_TICKS(30u * 1000u))
#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/**
 * @brief The internal states machine
 * of the OTA process.
 */
enum OTA_STATE
{
    /**No Download is in progress*/
    OTA_STATE_INACTIVE = 0,

    /**A Download is in progress*/
    OTA_STATE_ACTIVE,

    /**Download progress done, waiting for validation to complete*/
    OTA_STATE_DONE,

    /** Validation has completed waiting for CN MCY to store the binary*/
    OTA_STATE_VALIDATED
};
typedef enum OTA_STATE OTA_STATE;

/**
 * @brief The state of the binary
 */
enum OTA_BINARY_STATE
{
    OTABIN_STATE_DIRTY = 0,
    OTABIN_STATE_VALID,
};
typedef enum OTA_BINARY_STATE OTA_BINARY_STATE;

/** @brief The firmware binary types as indicated
 * in firmware distribution binary
 */
typedef enum CNGW_Firmware_Binary_Type
{
    CNGW_FIRMWARE_BINARY_TYPE_invalid = 0,
    CNGW_FIRMWARE_BINARY_TYPE_config = 1,
    CNGW_FIRMWARE_BINARY_TYPE_cn_mcu = 2,
    CNGW_FIRMWARE_BINARY_TYPE_sw_mcu = 3,
    CNGW_FIRMWARE_BINARY_TYPE_dr_mcu = 4,
    CNGW_FIRMWARE_BINARY_TYPE_gw_mcu = 5
} __attribute__((packed)) CNGW_Firmware_Binary_Type;

/**
 * @brief Status messages sent from the CN to GW
 */
typedef enum CNGW_Ota_Status
{
    /**!< CNGW_OTA_STATUS_Abort An unrecoverable
     * error occurred on the CN. GW will abort
     * the transmission and indicate to AWS
     * that OTA download failed.
     * CN can send more details error code
     * to GW via the Error system. These errors
     * will be forwarded directly to AWS.
     * */
    CNGW_OTA_STATUS_Error = 0,

    /**!< CNGW_OTA_STATUS_Success
     * CN MCU has successfully received and saved the
     * entire distribution binary to flash. GW will
     * use this status to indicate to AWS that
     * OTA download was successful.*/
    CNGW_OTA_STATUS_Success = 1,

    /**!< CNGW_OTA_STATUS_Resart
     * GW will restart the entire process
     * to send CN the distribution binary.
     * The download timer does not restart*/
    CNGW_OTA_STATUS_Restart = 2,

    /**!< CNGW_OTA_STATUS_Ack
     * CN MCU send this to GW after processing
     * an OTA message.
     * GW must not send any new data until this is
     * received.
     * If GW send 10 OTA message packed then
     * it must wait to receive 10 ACK
     * from CN before sending anymore data.
     * */
    CNGW_OTA_STATUS_Ack = 3,

} __attribute__((packed)) CNGW_Ota_Status;

/**
 * @brief Define the type of data within the one
 *        CNGW_Ota_Message_t message
 */
typedef enum CNGW_Ota_Command
{
    /**!< CNGW_OTA_CMD_File_Header_Info
     * File header field as indicated in distribution
     * binary documentation */
    CNGW_OTA_CMD_File_Header_Info = 1,

    /**!< CNGW_OTA_CMD_Package_Header_Info
     * Package header field as indicated in distribution
     * binary documentation */
    CNGW_OTA_CMD_Package_Header_Info = 2,

    /**!< CNGW_OTA_CMD_Crypto_Info
     * CNGW_Ota_Binary_Package_Header_t field
     * as indicated in distribution binary documentation*/
    CNGW_OTA_CMD_Crypto_Info = 3,

    /**!< CNGW_OTA_CMD_Binary_Data
     * The binary data after CNGW_Ota_Binary_Package_Header_t
     * as indicated in the distribution binary documentation*/
    CNGW_OTA_CMD_Binary_Data = 4,

    /**!< CNGW_OTA_CMD_Status
     * A status message form CN*/
    CNGW_OTA_CMD_Status = 5
} __attribute__((packed)) CNGW_Ota_Command;

/**
 * @brief File header layout
 */
struct OTA_FILE_HEADER
{
    uint8_t serial[9];
    uint8_t binary_count;
} __attribute__((packed));
typedef struct OTA_FILE_HEADER OTA_FILE_HEADER_t;

/**@brief Package header fields as indicated in the
 * distribution binary documentation
 * This is a byte by byte layout of how the data is packed
 * this layout must always match how the binary is built
 *  */
typedef struct CNGW_Ota_Binary_Package_Header_t
{
    CNGW_Firmware_Binary_Type type;
    CNGW_Firmware_Version_t version;
    uint32_t size;
    uint32_t crc;
} __attribute__((packed)) CNGW_Ota_Binary_Package_Header_t;

/**
 * @brief A single buffer.
 */
struct OTA_BUFFER
{
    uint8_t *mem;
    uint32_t size;
};
typedef struct OTA_BUFFER OTA_BUFFER_t;

/**
 * @brief Holds the data
 * related to the binary.
 */
struct OTA_BINARY
{
    OTA_BUFFER_t buffer;

    /** written_bytes Index to the location to start writing at
     * as well as the number byte written so far */
    uint32_t written_bytes;

    OTA_BINARY_STATE state;
};
typedef struct OTA_BINARY OTA_BINARY_t;


/**
 * @brief Holds the data
 * related to the current download
 * session
 */
struct OTA_SESSION
{
    TimeOut_t timer;
    uint32_t adjust_timeout_ticks;
    uint32_t init_timeout_ticks;

    /**The size of the binary
     * that the session is expected
     * to download*/
    uint32_t total_binary_size;
};
typedef struct OTA_SESSION OTA_SESSION_t;


/**
 * @brief Wrapper for binary_count in the distribution
 *  package with CRC.
 */
typedef struct CNGW_Ota_Binary_Count_Msg_t
{
    CNGW_Firmware_Version_t dist_release_version;
    uint8_t count;
    uint8_t crc;
} __attribute__((packed)) CNGW_Ota_Binary_Count_Msg_t;

/**
 * @brief Wrapper for @ref CNGW_Ota_Binary_Package_Header_t with CRC
 */
typedef struct CNGW_Ota_Binary_Package_Header_Msg_t
{
    CNGW_Ota_Binary_Package_Header_t package_header;
    uint8_t crc;
} __attribute__((packed)) CNGW_Ota_Binary_Package_Header_Msg_t;

/**
 * @brief
 * Data Flow: CN --> GW
 *
 *  CN replies to the GW with this status message.
 *  GW will check for status messages before sending the next
 *  messages. If no message is received GW will just
 *  continue sending data.
 *
 *  GW will take the appropriate action depending on
 *  the information in the message. @see CNGW_Ota_Status.
 *
 *  After the entire distribution binary is received CN must always
 *  reply with a status message. Which GW used to determined if everything
 *  was successful or not.
 *
 *  CN will know when the entire distribution binary is sent because
 *  it knows the number of binaries and the size of each binary.
 *  All this information is sent in the CNGW_Ota_Info_Message_t, thus
 *  it can keep track of things.
 *  However if the CN indicate CNGW_OTA_STATUS_Success before the GW
 *  has finished sending all the data. GW will just restart the process
 *  by sending a new CNGW_Ota_Info_Message_t with CNGW_OTA_CMD_File_Header_Info.
 *
 *  @note This is send under the header type #CNGW_HEADER_TYPE_Ota_Command
 */
typedef struct CNGW_Ota_Status_Message_t
{
    CNGW_Ota_Command command; /**@brief Sent using CNGW_OTA_CMD_Status*/
    CNGW_Ota_Status status;
    uint8_t crc;
} __attribute__((packed)) CNGW_Ota_Status_Message_t;


/**
 * @brief Cyrpo fields as indicated in the
 * distribution binary documentation
 * This is a byte by byte layout of how the data is packed
 * this layout must always match how the binary is built
 */
typedef struct CNGW_Ota_Cyrpto_Info_t
{
    uint8_t ecdsa[64];
    uint8_t random[32];
    uint8_t padding[32];
} __attribute__((packed)) CNGW_Ota_Cyrpto_Info_t;

/**
 * @brief Wrapper for @ref CNGW_Ota_Cyrpto_Info_t with CRC
 */
typedef struct CNGW_Ota_Cyrpto_Info_Msg_t
{
    CNGW_Ota_Cyrpto_Info_t cyrpto;
    uint8_t crc;
} __attribute__((packed)) CNGW_Ota_Cyrpto_Info_Msg_t;

/**@brief
 * Data Flow: GW --> CN
 *
 * Encrypted binary data as indicated in the distribution
 * binary. Use the CNGW_Message_Header_t::data_size to check the size of this message.
 * Only 128 bytes of binary can be sent.
 * This message structure is variable in length, dependant on
 * CNGW_Ota_Info_Message_t::encrypted_binary_and_crc Parameter size
 *
 * After sending 1 of these packets
 * the GW will go back into RX mode in case the CN wants to send a status
 * messages.
 *
 * After a full encrypted binary is sent GW will then transmit the next
 * CNGW_Ota_Binary_Package_Header_Msg_t message in a CNGW_Ota_Info_Message_t.
 *
 * This message must not contain any other field, array in wrapped
 * in the struct to keep the standard the same as other messages.
 *
 * The last byte in the @var encrypted_binary is always the crc
 * computed over the sent data in this structure.
 *
 * @attention The max size of encrypted_binary cannot
 * exceed (CN_SPI_RX_MAX_BUFFER_SIZE - 4)
 *
 * @note This is send under the header type #CNGW_HEADER_TYPE_Ota_Command
 * */
typedef struct CNGW_Ota_Binary_Data_Message_t
{
    CNGW_Ota_Command command; /**@brief Sent using CNGW_OTA_CMD_Binary_Data*/
    uint8_t encrypted_binary_and_crc[128 + 1];
} __attribute__((packed)) CNGW_Ota_Binary_Data_Message_t;

/**
 * @brief
 * Data Flow: GW  --> CN.
 * This message structure is variable in length, dependant on
 * CNGW_Ota_Info_Message_t::command used.
 *
 * Informational message related to the next
 * set of data that will be sent to the CN MCU
 * from the GW.
 *
 * This is sent when a new encrypted binary is about to be sent
 * or when a new distribution package is about to be sent.
 *
 * When ever the command is CNGW_OTA_CMD_File_Header_Info
 * the CN MCU should reset any internal states for download
 * a distribution package. If there an error on GW end while sending
 * a distribution package, GW will issue this command. CN must then honour
 * the command by restarting the process.
 *
 * @note This is send under the header type #CNGW_HEADER_TYPE_Ota_Command
 */
typedef struct CNGW_Ota_Info_Message_t
{
    CNGW_Ota_Command command;

    /**@brief
     * The supported messages per command.
     * Each messages has a CRC appended to the end
     * which must be calculated based on the
     * type selected from the union.
     *
     * The CRC calculation must include the
     * CNGW_Ota_Info_Message_t::command parameter*/
    union
    {
        /**< Sent when command is CNGW_OTA_CMD_File_Header_Info*/
        CNGW_Ota_Binary_Count_Msg_t binary_count_msg;

        /**< Send when command is CNGW_OTA_CMD_Package_Header_Info*/
        CNGW_Ota_Binary_Package_Header_Msg_t package_header_msg;

        /**< Send when command is CNGW_OTA_CMD_Crypto_Info*/
        CNGW_Ota_Cyrpto_Info_Msg_t crypto_msg;
    } u;
} __attribute__((packed)) CNGW_Ota_Info_Message_t;

/**
 * @brief A single OTA Informational frame sent over the wire.
 */
typedef struct CNGW_Ota_Info_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Ota_Info_Message_t message;
} __attribute__((packed)) CNGW_Ota_Info_Frame_t;

/**
 * @brief A single OTA binary frame sent over the wire.
 */
typedef struct CNGW_Ota_Binary_Data_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Ota_Binary_Data_Message_t message;
} __attribute__((packed)) CNGW_Ota_Binary_Data_Frame_t;

/**
 * @brief A single OTA status frame sent over the wires
 */
typedef struct CNGW_Ota_Status_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Ota_Status_Message_t message;
} __attribute__((packed)) CNGW_Ota_Status_Frame_t;

/**
 * @brief Pointer location to the various structure
 * within the distribution binary buffer.
 */
struct OTA_DISTRIBUTION_BINARY_CONTEXT
{
    int32_t binary_count;
    int32_t binary_size;

    const OTA_FILE_HEADER_t *file_header;
    const CNGW_Ota_Binary_Package_Header_t *package_header;
    const CNGW_Ota_Cyrpto_Info_t *crypto;
    const uint8_t *encyrpt_binary;
};
typedef struct OTA_DISTRIBUTION_BINARY_CONTEXT OTA_DIST_BIN_CTX_t;

#endif
#endif