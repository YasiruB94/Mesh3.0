#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CNGW_COMMON_H
#define CNGW_COMMON_H

#define GW_SPI_RX_MAX_BUFFER_SIZE (144u) /*Maximum RX DMA buffer for Gateway MCU. I.e Sizeof(CNGW_Log_Message_Frame_t)*/
#define RX_BUFFER_SIZE (GW_SPI_RX_MAX_BUFFER_SIZE * 4u)
#define CNGW_SERIAL_NUMBER_LENGTH (9U)
#define CNGW_CHALLENGE_RESPONSE_LENGTH (32U)
#define CNGW_HMAC_LENGTH (32U)


typedef enum CNGW_Source_MCU
{
  CNGW_SOURCE_MCU_CN = 0,
  CNGW_SOURCE_MCU_SW = 1,
  CNGW_SOURCE_MCU_GW = 2,

  /*These Driver field have to be linearly aligned
   * Because it's used in a loop count and
   * memory indexing on GW*/
  CNGW_SOURCE_MCU_DR0 = 3,
  CNGW_SOURCE_MCU_DR1 = 4,
  CNGW_SOURCE_MCU_DR2 = 5,
  CNGW_SOURCE_MCU_DR3 = 6,
  CNGW_SOURCE_MCU_DR4 = 7,
  CNGW_SOURCE_MCU_DR5 = 8,
  CNGW_SOURCE_MCU_DR6 = 9,
  CNGW_SOURCE_MCU_DR7 = 10,

  /*End marker indicating the last #
   * in the enum. Used for looping*/
  CNGW_SOURCE_MCU_END_MARKER
} __attribute__((packed)) CNGW_Source_MCU;

/**@brief This is the number of supported attribute CN
 * has. GW already support more attributes.
 * These values must be sequential in order
 * as it's directly casted from this enum
 * to the enum use on GW.
 * Never include derived attribute in here
 * attribute such as ON is derived from brightness
 * and auto added on the GW when it sees brightness attribute*/
typedef enum CNGW_Channel_Attribute_Type
{
  CNGW_ATTRIBUTE_Start_Marker = 0,
  CNGW_ATTRIBUTE_Brightness = 0x01,

  CNGW_ATTRIBUTE_End_Marker

} __attribute__((packed)) CNGW_Channel_Attribute_Type;

/**
 * @brief The command for each message
 */
typedef enum CNGW_Update_Command
{
  CNGW_UPDATE_CMD_Attribute = 0x01,
  CNGW_UPDATE_CMD_Status = 0x02,

} __attribute__((packed)) CNGW_Update_Command;

/**
 * @brief The status associated with a channel.
 *        Values are bit position into a 32bit value
 */
typedef enum CNGW_Update_Channel_Status
{
  CNGW_UPDATE_CHANNEL_STATUS_CNDR_FAULT_TYPE_START_MARKER = 1,

  /**@brief All values within CNGW_UPDATE_CHANNEL_STATUS_CNDR_FAULT_TYPE_XX_MARKER
   *        map directly to CNDR_FAULT_Type*/
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Light_Failure = CNGW_UPDATE_CHANNEL_STATUS_CNDR_FAULT_TYPE_START_MARKER, /**@brief One or more LEDs on the channel has failed.*/
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Short_Circuit = 2,
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Open_Circuit = 3,
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Temperature_Derate = 4,
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Temperature_Shotdown = 5,
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Volatage_Uncalibrated = 6, /**@brief No lights installed when driver booted*/

  CNGW_UPDATE_CHANNEL_STATUS_CNDR_FAULT_TYPE_END_MARKER = 7,

  /**@brief CNGW channel status values which have no relationship to CNDR_FAULT_Type*/
  CNGW_UPDATE_CHANNEL_STATUS_START_MARKER = CNGW_UPDATE_CHANNEL_STATUS_CNDR_FAULT_TYPE_END_MARKER,
  CNGW_UPDATE_CHANNEL_STATUS_Bit_Pos_Offline = CNGW_UPDATE_CHANNEL_STATUS_START_MARKER, /**@brief Channel Explicitly offline*/
  CNGW_UPDATE_CHANNEL_STATUS_END_MARKER

} __attribute__((packed)) CNGW_Update_Channel_Status;

/**
 * @brief The bit layout for the firmware version.
 */
typedef struct CNGW_Firmware_Version_t
{
  uint8_t major : 8;
  uint8_t minor : 8;
  uint32_t ci : 29;
  uint8_t branch_id : 3;
} __attribute__((packed)) CNGW_Firmware_Version_t;

/**
 * @brief The bit layout for the hardware version.
 */
struct CNGW_Hardware_Version_t
{
  uint8_t major;
  uint8_t minor;
  uint16_t ci;
} __attribute__((packed));
typedef struct CNGW_Hardware_Version_t CNGW_Hardware_Version_t;

/**@brief The enumeration of commands sent with each message
 * to identify the message*/

typedef enum CNGW_Device_Info_Command
{
  CNGW_DEVINFO_CMD_Invalid = 0x00,
  CNGW_DEVINFO_CMD_Update = 0x01,
  CNGW_DEVINFO_CMD_Remove = 0x02,
} __attribute__((packed)) CNGW_Device_Info_Command;

/**@brief Header type define a class of messages.
 *        These values are used for bit position shifting.
 *        The largest value must not exceed 32*/
typedef enum CNGW_Header_Type
{
  CNGW_HEADER_TYPE_Action_Commmand                  = 0x01,
  CNGW_HEADER_TYPE_Query_Command                    = 0x02,
  CNGW_HEADER_TYPE_Configuration_Command            = 0x03,
  CNGW_HEADER_TYPE_Configuration_Request_Command    = 0x04,
  CNGW_HEADER_TYPE_Config_Message_Command           = 0x05,
  CNGW_HEADER_TYPE_Handshake_Command                = 0x06,
  CNGW_HEADER_TYPE_Handshake_Response               = 0x07,
  CNGW_HEADER_TYPE_Firmware_Update_Command          = 0x08,
  CNGW_HEADER_TYPE_Status_Update_Command            = 0x09,
  CNGW_HEADER_TYPE_Log_Command                      = 0x0A,
  CNGW_HEADER_TYPE_Ota_Command                      = 0x0B,
  CNGW_HEADER_TYPE_Device_Report                    = 0x0C,
  CNGW_HEADER_TYPE_Control_Command                  = 0x0D,
  CNGW_HEADER_TYPE_Direct_Control_Command           = 0x0E,
  CNGW_HEADER_TYPE_End_Marker                       = 0x0F,

} __attribute__((packed)) CNGW_Header_Type;

/**
 * @brief Message Header structure used for all communication
 */
typedef struct CNGW_Message_Header_t
{
  CNGW_Header_Type command_type;

  /**@brief
   * The CNGW_Message_Header_t::data_size is in big endian format.
   * The value should be read and written to using the macros
   * *) CNGW_SET_HEADER_DATA_SIZE
   * *) CNGW_GET_HEADER_DATA_SIZE
   *
   * The sum of bytes that make up the message
   * that sent with header.
   * Some message have variable lengths, the structures
   * are used to define upper memory boundaries.
   * If a structure is of 128 byte, but only 30 bytes
   * were filled into this structure then this value is 30.
   * This can occour when messages that have variable lengths.
   */
  uint16_t data_size;
  uint8_t crc;

} __attribute__((packed)) CNGW_Message_Header_t;

/**@brief Status codes*/
typedef enum GW_STATUS
{
  /******************************************************************************
   * @brief Generic Status Codes
   ******************************************************************************/
  // GW_STATUS_GENERIC_ERROR = GET_GW_STATUS_CLASS_START_MARK(GENERIC),
  GW_STATUS_GENERIC_SUCCESS,
  GW_STATUS_GENERIC_TIMEOUT,
  GW_STATUS_GENERIC_BAD_PARAM,

  /******************************************************************************
   * @brief AWS Status Codes
   ******************************************************************************/
  // GW_STATUS_AWS_TOPIC_ID_ERR = GET_GW_STATUS_CLASS_START_MARK(AWS),
  GW_STATUS_AWS_BUFFER_TOO_SMALL_ERR,
  GW_STATUS_AWS_PB_ERR,
  GW_STATUS_AWS_CERT_ERR,
  GW_STATUS_AWS_CERT_DEVICE_ERR,
  GW_STATUS_AWS_CERT_SIGNER_ERR,

  // ADD_GW_STATUS_ERR_END_MARKER(AWS),

  /****/
  GW_STATUS_AWS_NETWORK_PHYSICAL_LAYER_CONNECTED, //!< GW_STATUS_AWS_NETWORK_PHYSICAL_LAYER_CONNECTED
  GW_STATUS_AWS_NETWORK_MANUALLY_DISCONNECTED,    //!< GW_STATUS_AWS_NETWORK_MANUALLY_DISCONNECTED
  GW_STATUS_AWS_NETWORK_ATTEMPTING_RECONNECT,     //!< GW_STATUS_AWS_NETWORK_ATTEMPTING_RECONNECT
  GW_STATUS_AWS_NETWORK_RECONNECTED,              //!< GW_STATUS_AWS_NETWORK_RECONNECTED
  GW_STATUS_AWS_MQTT_NOTHING_TO_READ,             //!< GW_STATUS_AWS_MQTT_NOTHING_TO_READ
  GW_STATUS_AWS_MQTT_CONNACK_CONNECTION_ACCEPTED, //!< GW_STATUS_AWS_MQTT_CONNACK_CONNECTION_ACCEPTED
  GW_STATUS_AWS_JSON_MORE,                        //!< GW_STATUS_AWS_JSON_MORE Indicate the the JSON document has more things to serialize
  GW_STATUS_AWS_JSON_NONE,                        //!< GW_STATUS_AWS_JSON_NONE No JSON to be serialized

  /******************************************************************************
   * @brief Device Info Status Codes
   ******************************************************************************/
  // GW_STATUS_DEVINFO_INVALID_FIELD_ERR = GET_GW_STATUS_CLASS_START_MARK(DEVINFO),

  // ADD_GW_STATUS_ERR_END_MARKER(DEVINFO),

  /****/
  GW_STATUS_DEVINFO_STATE_UPDATE, //!< GW_STATUS_DEVINFO_STATE_UPDATE Only state field was set to update on the cache update
  GW_STATUS_DEVINFO_NO_UPDATE,

  /******************************************************************************
   * @brief OTA Status Codes
   ******************************************************************************/
  // GW_STATUS_OTA_ALLOC_ERR = GET_GW_STATUS_CLASS_START_MARK(OTA),
  GW_STATUS_OTA_BIG_BLOCK_ERR,
  GW_STATUS_OTA_FRAME_TYPE_ERR,
  GW_STATUS_OTA_NO_BYTES_EXPECTED_ERR,
  GW_STATUS_OTA_NO_ACTIVE_SESSION_ERR,
  GW_STATUS_OTA_DOWNLOAD_SYNC_ERR, //!< GW_STATUS_OTA_DOWNLOAD_SYNC_ERR Indicate that the the tx_byte_count != written_bytes.
  GW_STATUS_OTA_SHA256_ERR,
  GW_STATUS_OTA_VALIDATION_TIMEOUT_ERR,
  GW_STATUS_OTA_BINARY_COUNT_ERR,
  GW_STATUS_OTA_ECDSA_SIGNATURE_ERR,
  GW_STATUS_OTA_CN_SERIAL_ERR,
  GW_STATUS_OTA_INVALID_BINARY_TYPE_ERR,
  GW_STATUS_OTA_CRC32_ERR,
  GW_STATUS_OTA_BINARY_SIZE_ERR, //!< GW_STATUS_OTA_BINARY_SIZE_ERR The SUM of the binary data does not match the amount of data downloaded.
  GW_STATUS_OTA_SAVE_TIMEOUT_ERR,
  GW_STATUS_OTA_SAVE_ERR,
  GW_STATUS_OTA_DOWNLOAD_TIMEOUT_ERR,
  GW_STATUS_OTA_DOWNLOAD_ERR,

  // ADD_GW_STATUS_ERR_END_MARKER(OTA),

  /****/
  GW_STATUS_OTA_SAVE_DONE,
  GW_STATUS_OTA_DOWNLOAD_DONE,
  GW_STATUS_OTA_VALIDATION_DONE,

  /******************************************************************************
   * @brief CCP Status Codes
   ******************************************************************************/
  // ADD_GW_STATUS_ERR_END_MARKER(CCP),

  /****/
  GW_STATUS_CCP_DUMMY_PACKET,
  GW_STATUS_CCP_FRAME_LOCK,

  /******************************************************************************
   * @brief Channel Status Codes
   ******************************************************************************/
  // GW_STATUS_CHNL_VALIDATE_ERR = GET_GW_STATUS_CLASS_START_MARK(CHNL),
  GW_STATUS_CHNL_VALIDATE_NAME_ERR,
  GW_STATUS_CHNL_VALIDATE_ATTR_ERR,
  GW_STATUS_CHNL_MSG_SIZE_ERR,           //!< GW_STATUS_CHNL_MSG_SIZE_ERR The received messages size is not expected
  GW_STATUS_CHNL_MSG_PARAMETER_ERR,      //!< GW_STATUS_CHNL_MSG_PARAMETER_ERR There is a parameter error on the channel that was received from CN
  GW_STATUS_CHNL_CONFIG_NUMBER_HASH_ERR, //!< GW_STATUS_CHNL_CONFIG_NUMBER_HASH_ERR

  // ADD_GW_STATUS_ERR_END_MARKER(CHNL),

  /****/
  GW_STATUS_CHNL_ENTRY_END,          //!< GW_STATUS_CHNL_ENTRY_END No more entries exist
  GW_STATUS_CHNL_WAIT_CN_STATUS_MSG, //!< GW_STATUS_CHNL_WAIT_CN_STATUS_MSG Waiting on CN to send a status message, caller should sleep for a bit when this is received

  /******************************************************************************
   * @brief Atmel Status Codes
   ******************************************************************************/
  //  GW_STATUS_ATMEL_SHA1_ERR = GET_GW_STATUS_CLASS_START_MARK(ATMEL),
  GW_STATUS_ATMEL_SHA256_ERR,
  GW_STATUS_ATMEL_CERT_DECODING_ERR,
  GW_STATUS_ATMEL_CERT_ENCODING_ERR,

  // ADD_GW_STATUS_ERR_END_MARKER(ATMEL),

  /****/
  GW_STATUS_ATMEL_CERT_BUFFER_TOO_SMALL,     //!< GW_STATUS_ATMEL_CERT_BUFFER_TOO_SMALL
  GW_STATUS_ATMEL_CERT_INVALID_DATE,         //!< GW_STATUS_ATMEL_CERT_INVALID_DATE
  GW_STATUS_ATMEL_CERT_UNEXPECTED_ELEM_SIZE, //!< GW_STATUS_ATMEL_CERT_UNEXPECTED_ELEM_SIZE
  GW_STATUS_ATMEL_CERT_ELEM_MISSING,         //!< GW_STATUS_ATMEL_CERT_ELEM_MISSING
  GW_STATUS_ATMEL_CERT_ELEM_OUT_OF_BOUNDS,   //!< GW_STATUS_ATMEL_CERT_ELEM_OUT_OF_BOUNDS
  GW_STATUS_ATMEL_CERT_BAD_CERT,             //!< GW_STATUS_ATMEL_CERT_BAD_CERT
  GW_STATUS_ATMEL_CERT_WRONG_CERT_DEF,       //!< GW_STATUS_ATMEL_CERT_WRONG_CERT_DEF
  GW_STATUS_ATMEL_CERT_VERIFY_FAILED,        //!< GW_STATUS_ATMEL_CERT_VERIFY_FAILED

  /*Try to keep atecc508 error code by off setting them by ATCA_TO_ATMEL_START_MARK*/
  ATCA_TO_ATMEL_START_MARK,
  GW_STATUS_ATMEL_CONFIG_ZONE_LOCKED = ATCA_TO_ATMEL_START_MARK + 0x01,
  GW_STATUS_ATMEL_DATA_ZONE_LOCKED = ATCA_TO_ATMEL_START_MARK + 0x02,
  GW_STATUS_ATMEL_WAKE_FAILED = ATCA_TO_ATMEL_START_MARK + 0xD0,
  GW_STATUS_ATMEL_CHECKMAC_VERIFY_FAILED = ATCA_TO_ATMEL_START_MARK + 0xD1,
  GW_STATUS_ATMEL_PARSE_ERROR = ATCA_TO_ATMEL_START_MARK + 0xD2,
  GW_STATUS_ATMEL_CRC = ATCA_TO_ATMEL_START_MARK + 0xD4,
  GW_STATUS_ATMEL_UNKNOWN = ATCA_TO_ATMEL_START_MARK + 0xD5,
  GW_STATUS_ATMEL_ECC = ATCA_TO_ATMEL_START_MARK + 0xD6,
  GW_STATUS_ATMEL_FUNC_FAIL = ATCA_TO_ATMEL_START_MARK + 0xE0,
  GW_STATUS_ATMEL_INVALID_ID = ATCA_TO_ATMEL_START_MARK + 0xE3,
  GW_STATUS_ATMEL_INVALID_SIZE = ATCA_TO_ATMEL_START_MARK + 0xE4,
  GW_STATUS_ATMEL_BAD_CRC = ATCA_TO_ATMEL_START_MARK + 0xE5,
  GW_STATUS_ATMEL_RX_FAIL = ATCA_TO_ATMEL_START_MARK + 0xE6,
  GW_STATUS_ATMEL_RX_NO_RESPONSE = ATCA_TO_ATMEL_START_MARK + 0xE7,
  GW_STATUS_ATMEL_TX_TIMEOUT = ATCA_TO_ATMEL_START_MARK + 0xEA,
  GW_STATUS_ATMEL_RX_TIMEOUT = ATCA_TO_ATMEL_START_MARK + 0xEB,
  GW_STATUS_ATMEL_BAD_OPCODE = ATCA_TO_ATMEL_START_MARK + 0xF2,
  GW_STATUS_ATMEL_WAKE_SUCCESS = ATCA_TO_ATMEL_START_MARK + 0xF3,
  GW_STATUS_ATMEL_EXECUTION_ERROR = ATCA_TO_ATMEL_START_MARK + 0xF4,
  GW_STATUS_ATMEL_NOT_LOCKED = ATCA_TO_ATMEL_START_MARK + 0xF8,

  /******************************************************************************
   * @brief DDD Status Codes
   ******************************************************************************/
  //  GW_STATUS_DDD_ATTR_DISABLE_ERR = GET_GW_STATUS_CLASS_START_MARK(DDD),         //!< GW_STATUS_DDD_ATTR_DISABLE_ERR Attribute is disabled. Check configuration
  GW_STATUS_DDD_ATTR_DUPLICATE_ERR,   //!< GW_STATUS_DDD_ATTR_DUPLICATE_ERR Attribute duplicated within 1 service. Check configuration
  GW_STATUS_DDD_ATTR_UNKNOWN_ERR,     //!< GW_STATUS_DDD_ATTR_UNKNOWN_ERR The attribute you are trying to access is unknown to the service.
  GW_STATUS_DDD_SERVICE_UNKNOWN_ERR,  //!< GW_STATUS_DDD_SERVICE_UNKNOWN_ERR The service is unknown to the system
  GW_STATUS_DDD_ATTR_VALUE_RANGE_ERR, //!< GW_STATUS_DDD_ATTR_VALUE_RANGE_ERR The attribute is not within the min/max range of the attribute
  GW_STATUS_DDD_PAREMETER_ERR,        //!< GW_STATUS_DDD_PAREMETER_ERR Input parameter to the function is invalid
  GW_STATUS_DDD_KEY_DUPLICATE_ERR,    //!< GW_STATUS_DDD_KEY_DUPLICATE_ERR, Tried to insert the same key more than once.
  GW_STATUS_DDD_RANGE_ERR,            //!< GW_STATUS_DDD_RANGE_ERR Min value > max value
  GW_STATUS_DDD_STATE_ERR,            //!< GW_STATUS_DDD_STATE_ERR The database is not in the correct state to allow for the method you are trying to use
  GW_STATUS_DDD_NO_ENTRY_ERR,         //!< GW_STATUS_DDD_NO_ENTRY_ERR

  // ADD_GW_STATUS_ERR_END_MARKER(DDD),

  /****/
  GW_STATUS_DDD_NO_UPDATE, //!< GW_STATUS_DDD_NO_UPDATE An Update was not required

  /******************************************************************************
   * @brief HKAPP Status Codes
   ******************************************************************************/
  //   GW_STATUS_HKAPP_ACC_LIMIT_ERR = GET_GW_STATUS_CLASS_START_MARK(HKAPP),            //!< GW_STATUS_HKAPP_ACC_LIMIT_ERR There more primary entries than the was configured in the external flash. The "Total Primary Entries" is wrong in flash
  GW_STATUS_HKAPP_ACC_BRDGE_ERR,     //!< GW_STATUS_HKAPP_ACC_BRDGE_ERR Bridge initialization error
  GW_STATUS_HKAPP_SERV_LIMIT_ERR,    //!< GW_STATUS_HKAPP_SERV_LIMIT_ERR There more service entries than was configured for the primary entry in external flash. The "Adjacent entry count" is wrong in flash
  GW_STATUS_HKAPP_SERV_INVALID_ERR,  //!< GW_STATUS_HKAPP_SERV_INVALID_ERR This service is not supported by the firmware
  GW_STATUS_HKAPP_CHARX_INVALID_ERR, //!< GW_STATUS_HKAPP_CHARX_INVALID_ERR This characteristic is not supported by the firmware
  GW_STATUS_HKAPP_CHARX_RANGE_ERR,   //!< GW_STATUS_HKAPP_CHARX_RANGE_ERR The min value is > the max value
  GW_STATUS_HKAPP_CHARX_DUPL_ERR,    //!< GW_STATUS_HKAPP_CHARX_DUPL_ERR Duplicate attribute were in the entry initialization list
  GW_STATUS_HKAPP_CHARX_MISS_ERR,    //!< GW_STATUS_HKAPP_CHARX_MISS_ERR The charx is missing from the configuration it's required for basic operation

  // ADD_GW_STATUS_ERR_END_MARKER(HKAPP),

  /******************************************************************************
   * @brief HI OLED Status Codes
   ******************************************************************************/
  //   GW_STATUS_HIOLED_Y_ERR = GET_GW_STATUS_CLASS_START_MARK(HIOLED),          //!< GW_STATUS_HIOLED_Y_ERR Invalid Y axis parameter
  GW_STATUS_HIOLED_X_ERR,      //!< GW_STATUS_HIOLED_X_ERR Invalid X axis parameter
  GW_STATUS_HIOLED_COLOUR_ERR, //!< GW_STATUS_HIOLED_COLOUR_ERR
  GW_STATUS_HIOLED_INDEX_ERR,  //!< GW_STATUS_HIOLED_INDEX_ERR

  //  ADD_GW_STATUS_ERR_END_MARKER(HIOLED),

  /******************************************************************************
   * @brief IP STATUS
   ******************************************************************************/
  //  GW_STATUS_IP_SOCK_ERR = GET_GW_STATUS_CLASS_START_MARK(IP),           //!< GW_STATUS_IP_SOCK_ERR Some kind of socket error related to fnet stack. Only option is to close the connection and restablish it
  GW_STATUS_IP_TLS_HANDSHAKE_ERR, //!< GW_STATUS_IP_TLS_HANDSHAKE_ERR The Handshake process failed. Further Diagnostic will be required to validate why it failed
  GW_STATUS_IP_GET_DEVICE_PUBLIC_KEY_ERR,
  GW_STATUS_IP_DNS_ERR, //!< GW_STATUS_IP_DNS_ERR The DNS hostname could not be resolved after trying to search

  //   ADD_GW_STATUS_ERR_END_MARKER(IP),

  /****/
  GW_STATUS_IP_TLS_CERT_PARSE_PARTIAL, //!< GW_STATUS_IP_TLS_CERT_PARSE_PARTIAL Only some of the certificate that were set to parse succeeded
  GW_STATUS_IP_TLS_NOT_CONNECTED,      //!< GW_STATUS_IP_TLS_NOT_CONNECTED TLS
  GW_STATUS_IP_NO_IP,                  //!< GW_STATUS_IP_NO_IP

  /******************************************************************************
   * @brief File System STATUS
   ******************************************************************************/
  //  GW_STATUS_FS_CRC_ERR = GET_GW_STATUS_CLASS_START_MARK(FS),
  GW_STATUS_FS_RECORD_CORRUPT_ERR,

  //    ADD_GW_STATUS_ERR_END_MARKER(FS),

  /****/
  GW_STATUS_FS_INVD_SIZE,
  GW_STATUS_FS_FULL,
  GW_STATUS_FS_NOREC,

  /******************************************************************************
   * @brief Interrupt Timer STATUS
   ******************************************************************************/
  //  ADD_GW_STATUS_ERR_END_MARKER(IT),

  /****/
  GW_STATUS_IT_USE_RTOS_DELAY, //!< GW_STATUS_IT_USE_RTOS_DELAY The delays specified can be done using the RTOS delay method
  GW_STATUS_IT_NOT_TRIGGERED,  //!< GW_STATUS_IT_NOT_TRIGGERED The hardware timer did not trigger in a reasonable time, it was forced stop

  /******************************************************************************
   * @brief Linked List Status
   ******************************************************************************/
  //  ADD_GW_STATUS_ERR_END_MARKER(LL),

  /****/
  GW_STATUS_LL_ITERATOR_NEXT,
  GW_STATUS_LL_ITERATOR_STOP,

  /* *****************************************************************************
   * NO EDITING BELOW HERE
   ******************************************************************************/
  GW_STATUS_END_MARKER //!< GW_STATUS_END_MARKER
} GW_STATUS;

#endif
#endif