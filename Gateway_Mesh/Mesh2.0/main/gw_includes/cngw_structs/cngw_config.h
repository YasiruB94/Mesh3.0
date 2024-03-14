#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_CONFIG_H
#define CNGW_CONFIG_H

#define MAX_SENSORS                     (32)
#define MAX_SENSOR_COMMANDS             (2)
#define MAX_SENSOR_TARGETS              (3)
#define SENSOR_OCCUPIED_COMMAND_INDEX   (0)
#define SENSOR_VACANT_COMMAND_INDEX     (1)
#define LM1_CHANNEL_COUNT 4U

#include "cngw_common.h"
/**
 * @brief Log levels which must
 * match CNGW_Log_Level
 */
typedef enum Logger_Level
{
    LOG_DISABLE     = 0,
	LOG_ERROR       = 1,     /*Default for all MCUs*/
	LOG_WARNING     = 2,
	LOG_INFO        = 3,
	LOG_DEBUG       = 4,
	LOG_VERBOSE     = 5,
	LOG_End_Marker

} CENSE_Logger_Level;

/**
 * @brief Reasons that an MCU (DR or CN) has reset
 *
 * @note Be careful if you re-order these
 */
typedef enum MCU_Reset_Reason
{
    MCU_RESET_REASON_None = 0,
    MCU_RESET_REASON_Power_On_Reset,                   //!< MCU_RESET_REASON_Power_On_Reset
    MCU_RESET_REASON_Reset_Button,                         //!< MCU_RESET_REASON_Reset_Button
    MCU_RESET_REASON_Option_Byte_Reload,                   //!< MCU_RESET_REASON_Option_Byte_Reload
    MCU_RESET_REASON_Low_Power_Reset,                      //!< MCU_RESET_REASON_Low_Power_Reset
    MCU_RESET_REASON_Other,                                //!< MCU_RESET_REASON_Other

    // ALL ITEMS after this are statuses that will show as a watchdog reset
    MCU_RESET_REASON_Watchdog_Reset,                       //!< MCU_RESET_REASON_Watchdog_Reset
    MCU_RESET_REASON_Hard_Fault,                           //!< MCU_RESET_REASON_Hard_Fault
    MCU_RESET_REASON_Firmware_Update,                      //!< MCU_RESET_REASON_Firmware_Update
    MCU_RESET_REASON_Watchdog_Reset_End_Marker,
    // ALL ITEMS after this are statuses that will show as software reset
    MCU_RESET_REASON_Software_Reset,                       //!< MCU_RESET_REASON_Software_Reset
    MCU_RESET_REASON_Software_SPI_CNDR_Communication_Issue,//!< MCU_RESET_REASON_Software_SPI_CNDR_Communication_Issue
    MCU_RESET_REASON_Software_Main_Error_Handler,
    MCU_RESET_REASON_Software_DR_Invalid_Module,
    MCU_RESET_REASON_Software_Reset_End_Marker

} MCU_Reset_Reason;

enum
{
    WIRED_SWITCH_COUNT = 32,
    WIRELESS_CONTACTS_PER_SWITCH = 4,
    WIRELESS_SWITCH_COUNT = 32 * WIRELESS_CONTACTS_PER_SWITCH,
    MAX_SWITCH_TARGETS = 3,
    MAX_SWITCH_COMMANDS = 4,
    MAX_CENSE_GROUPS = 256,
    MAX_DALI_GROUPS = 15,
    MAX_CENSE_SCENSE = 256,
    MAX_DALI_SCENES = 15,
    MAX_DALI_ADDRESSES = 64
};

enum
{
	CABINET__DRIVER_SLOT_COUNT = 8,
	DRIVER__MAX_CHANNELS_PER_DRIVER = LM1_CHANNEL_COUNT,
	DRIVER__MAX_BONDED_CHANNELS = 4,
	DRV_MAX_LIGHT_LEVEL = 2047,
	DRV_MAX_CURRENT     = 1800,

	/** Used to determine at what level a Dim UP is used over a Dim Down */
	SW_PRG_TOGGLE_UPDOWN__THRESHHOLD = (DRV_MAX_LIGHT_LEVEL >> 1) + 1 // 50% + 1
};


typedef enum CNSW_SPI_BF_Switch_Type
{
    CNSW_SWITCH_TYPE_Standard_Switch         = 0x0,
    CNSW_SWITCH_TYPE_Momentary_Switch        = 0x1,

} __attribute__((packed)) CNSW_SPI_BF_Switch_Type;

typedef enum CNSW_Enocean_Device_Type
{
    CNSW_ENOCEAN_DEVICE_TYPE_Not_Used                = 0x0,
    CNSW_ENOCEAN_DEVICE_TYPE_Switch                  = 0x1,
    CNSW_ENOCEAN_DEVICE_TYPE_Sensor_Occupancy        = 0x2,
    CNSW_ENOCEAN_DEVICE_TYPE_Sensor_Light_Harvesting = 0x3,
    CNSW_ENOCEAN_End_Marker

} __attribute__((packed)) CNSW_Enocean_Device_Type;

enum CN_DRV_Slot_Status
{
	/** @brief Driver slot is not populated */
	CN_SLOT__STATUS_EMPTY = 0,
	/** @brief Driver is in handshake CN1 status */
	CN_SLOT__STATUS_HANDSHAKE_CN1,
	/** @brief Driver is in handshake CN2 status*/
	CN_SLOT__STATUS_HANDSHAKE_CN2,
	/** @brief Driver is preparing for a firmware update */
	CN_SLOT__STATUS_PENDING_FIRMWARE_UPDATE,
	/** @brief Driver is in the process of updating it's firmware */
	CN_SLOT__STATUS_UPDATING_FIRMWARE,
	/** @brief Driver is online and functioning normally */
	CN_SLOT__STATUS_ONLINE,
	/** @brief Whole driver is in fault status */
	CN_SLOT__STATUS_DRV_FAULT,
	/** @brief Driver has failed handshake process */
	CN_SLOT__STATUS_INVALID_MODULE

};

enum CN_DRV_Type
{
	CN_DRV_TYPE__CONSTANT_CURRENT
//	CN_DRV_TYPE__CONSTANT_VOLTAGE
};

enum CN_DRV_Channel_Bond
{

	CN_BONDED__CHANNEL_INDIPENDANT = 0,
	CN_BONDED__CHANNEL_BONDED = 1

};

enum CN_DRV_Channel_Configured
{
	CN_CHANNEL__STATUS_INACTIVE = 0,
	CN_CHANNEL__STATUS_ACTIVE = 1
};

enum CN_DRV_Slot_Config_Status
{
	/** @brief Configuration information does not exist for this slot */
	CN_SLOT_CONFIGURATION__DOES_NOT_EXIST = 0,
	/** @brief Configuration information exists for this slot */
	CN_SLOT_CONFIGURATION__EXISTS = 1
};

typedef enum SENSOR_Type
{
    SENSOR_TYPE_Occupancy        = CNSW_ENOCEAN_DEVICE_TYPE_Sensor_Occupancy,
    SENSOR_TYPE_LIGHT_HARVESTING = CNSW_ENOCEAN_DEVICE_TYPE_Sensor_Light_Harvesting

} SENSOR_Type;

/**
 * @brief Represents a type of trigger that can occur on a switch source.
 */
typedef enum CNSW_Action_Trigger
{
    CNSW_ACTION_TRIGGER_NONE            = 0x0,//!< CNSW_ACTION_TRIGGER_NONE
    CNSW_ACTION_TRIGGER_SINGLE_PRESS    = 0x1,//!< CNSW_ACTION_TRIGGER_SINGLE_PRESS
    CNSW_ACTION_TRIGGER_DOUBLE_PRESS    = 0x2,//!< CNSW_ACTION_TRIGGER_DOUBLE_PRESS
    CNSW_ACTION_TRIGGER_TRIPLE_PRESS    = 0x3,//!< CNSW_ACTION_TRIGGER_TRIPLE_PRESS
    CNSW_ACTION_TRIGGER_HOLD_START      = 0x4,//!< CNSW_ACTION_TRIGGER_HOLD_START
    CNSW_ACTION_TRIGGER_HOLD_REPEAT     = 0x5,//!< CNSW_ACTION_TRIGGER_HOLD_REPEAT
    CNSW_ACTION_TRIGGER_HOLD_END        = 0x6,//!< CNSW_ACTION_TRIGGER_HOLD_END
    CNSW_ACTION_TRIGGER_STANDARD_ON     = 0x7,//!< CNSW_ACTION_TRIGGER_STANDARD_ON
    CNSW_ACTION_TRIGGER_STANDARD_OFF    = 0x8, //!< CNSW_ACTION_TRIGGER_STANDARD_OFF
    CNSW_ACTION_TRIGGER_DALI_CMD        = 0x9,
    CNSW_ACTION_TRIGGER_Sensor_Occupied = 0xA,
    CNSW_ACTION_TRIGGER_Sensor_Vacant   = 0xB,
    CNSW_ACTION_TRIGGER_End_Marker
} __attribute__((packed)) CNSW_Action_Trigger;

typedef enum CNDR_SPI_Action_Command
{
    CNDR_SPI_ACTION__NO_ACTION          = 0x0,
    CNDR_SPI_ACTION__ON_FULL            = 0x1,
    CNDR_SPI_ACTION__ON_AT_LEVEL_X      = 0x2,
    CNDR_SPI_ACTION__OFF                = 0x3,
    CNDR_SPI_ACTION__EXECUTE_SCENE      = 0x4,
    CNDR_SPI_ACTION__DAYLIGHT_HARVESTING= 0x5,
    CNDR_SPI_ACTION__TUNE_2CH_CCT_COOLER= 0x6,
    CNDR_SPI_ACTION__TUNE_2CH_CCT_WARMER= 0x7,
    CNDR_SPI_ACTION__DELAY_OFF          = 0xA,
    CNDR_SPI_ACTION__ON_LAST_LEVEL      = 0xB,
    CNDR_SPI_ACTION__UP                 = 0xC,
    CNDR_SPI_ACTION__DOWN               = 0xD,
    CNDR_SPI_ACTION__TOGGLE_ONOFF       = 0xE,

    // INTERMEDIATE ACTION - this needs to be translated to UP or DOWN action.
    CNDR_SPI_ACTION__TOGGLE_UPDOWN      = 0xF,
    CNDR_SPI_ACTION__TOGGLE_TUNE_2CH_CCT= 0x1F,

    CNDR_SPI_ACTION__END_MARKER

} CNDR_SPI_Action_Command;

enum DRV_Target_Type
{
    DRV_TARGET_TYPE__UNUSED                 = 0x00,
    DRV_TARGET_TYPE__CENSE_SINGLE_ADDRESS   = CNGW_ADDRESS_TYPE_Cense_Channel,
    DRV_TARGET_TYPE__CENSE_GROUP_ADDRESS    = CNGW_ADDRESS_TYPE_Cense_Group,
    DRV_TARGET_TYPE__CENSE_SCENE            = CNGW_ADDRESS_TYPE_Cense_Scene,
    DRV_TARGET_TYPE__CENSE_BROADCAST        = CNGW_ADDRESS_TYPE_Cense_Broadcast,
    DRV_TARGET_TYPE__DALI_SINGLE_ADDRESS    = CNGW_ADDRESS_TYPE_Dali_Address,
    DRV_TARGET_TYPE__DALI_GROUP_ADDRESS     = CNGW_ADDRESS_TYPE_Dali_Group,
    DRV_TARGET_TYPE__DALI_BROADCAST         = CNGW_ADDRESS_TYPE_Dali_Broadcast
};

enum SW_Type
{
    SW_TYPE__STANDARD = CNSW_SWITCH_TYPE_Standard_Switch,
    SW_TYPE__MOMENTARY = CNSW_SWITCH_TYPE_Momentary_Switch

} ;

/**
 * Defines the type of fixture connected to a channel
 */
typedef enum CNDR_Fixture_Type
{
    CNDR_FIXTURE_TYPE__WHITE            = 0x00,//!< CNDR_FIXTURE_TYPE__WHITE
    CNDR_FIXTURE_TYPE__2CH_CCT          = 0x01 //!< CNDR_FIXTURE_TYPE__2CH_CCT

} __attribute__((packed)) CNDR_Fixture_Type;


typedef CNSW_Action_Trigger SW_Trigger;

struct SW_Command_t { // 6 bytes

	SW_Trigger              eventTrigger : 4;
	uint8_t                 reserved     : 4;
	CNDR_SPI_Action_Command action       : 8;
	uint32_t                parameters;

} __attribute__((packed)) ;
typedef struct SW_Command_t * SW_Command_p;

struct CN_DRV_Target_t
{ // 6 bytes
	enum DRV_Target_Type targetType : 8;
	     uint16_t        cabinetNumber : 9;
	     uint8_t         slot : 3;
	     uint8_t         channel;
	     uint8_t         group;
	     uint8_t         scene;
}__attribute__((packed)) ;
typedef struct CN_DRV_Target_t * CN_DRV_Target_p;


struct CN_Switch_Configuration_t
{ // 56 bytes (32 switches = 1632 bytes) 8160

	uint16_t                parameters; 						// 2 bytes
	enum SW_Type            type : 4;  							// 1/2 byte (really only 1 bit)
	SW_Trigger              lastReceivedTrigger : 4;			// 1/2 byte
	struct SW_Command_t     commands[MAX_SWITCH_COMMANDS]; 	// 4 x 6 bytes
	struct CN_DRV_Target_t  targets[MAX_SWITCH_TARGETS]; 		// 3 x 6 bytes
	uint32_t                id;
	// bit 0 LSB (SW_FLAGS__SPECIAL_PROCESSING) defines if special processing is needed
	//      (if there is a TOGGLE_UPDOWN command this must be set)
	// bit 1 LSB (SW_FLAGS__UPDOWN) is for UP / DOWN direction for TOGGLE_UPDOWN action. (0 == UP, 1 == DOWN)
	uint8_t                 flags; // can be used to store info during use.

}__attribute__((packed));
typedef struct CN_Switch_Configuration_t * CN_Switch_Configuration_p;

typedef struct CN_Sensor_Configuration_t
{
    union
    {
        uint16_t            desired_lux;
        uint16_t            occupancy_timeout_minutes;
    } parameters;

    SENSOR_Type             type : 4;
    uint32_t                last_trigger_tick;                  // systick of last trigger
    struct SW_Command_t     commands[MAX_SENSOR_COMMANDS];     // 4 x 5 bytes
    struct CN_DRV_Target_t  targets[MAX_SENSOR_TARGETS];        // 3 x 6 bytes
    uint32_t                id;

}__attribute__((packed)) CN_Sensor_Configuration_t;

// DO NOT CHANGE THE ORDER OF THIS STRUCTURE
// new variables can be added to the end if needed.
// this structure is serialized and de-serialized to/from EEPROM as well as
// used in transmitting metrics to Control MCU
struct LM1_Channel_Metrics_t
{
	uint32_t output_seconds;
	union
	{
		float 	 watts_hours_float;
		uint32_t watts_hours;
	};
} __attribute__((packed));

// DO NOT CHANGE THE ORDER OF THIS STRUCTURE
// new variables can be added to the end if needed.
// this structure is serialized and de-serialized to/from EEPROM as
// used in transmitting metrics to Control MCU
/**
 * @brief Structure to track operational metrics
 *
 * @note If adding to this structure and sizeof(struct) > 32 then
 * 			function #METRIC_Write_Metrics_To_Flash will need to be updated
 *
 * @note The maximum size this structure can be is 64 bytes otherwise it won't
 * 		 fit in the backup registers
 */
typedef struct LM1_Driver_Metrics_t
{
	uint32_t operational_minutes;
	uint16_t max_temperature;
	uint32_t temperature_zone_1_seconds;
	uint32_t temperature_zone_2_seconds;
	uint32_t temperature_zone_3_seconds;
	uint32_t temperature_zone_4_seconds;
	struct LM1_Channel_Metrics_t channels[LM1_CHANNEL_COUNT];

} __attribute__((packed)) LM1_Driver_Metrics_t;


//** Used to define a driver channel */
typedef struct CN_DRV_Channel_Config_t
{
	/** Channel Number ( 0 - DRIVER_MAX_CHANNELS_PER_DRIVER ) */
	uint8_t  channel : 4;
	/** Defines if channel is active or not (has lights hooked up) */
	enum CN_DRV_Channel_Configured active : 1;
	/** The Slot Number of this driver channel */
	uint8_t  slot : 3;
	/** The type of fixtures attached to the channel (White, CCT) */
	CNDR_Fixture_Type configured_type : 3;
    /** The maximum current for the channel */
    uint16_t current;
    /**
     * This is the initial starting current, for standard fixture types this is
     * set to the max current. For CCT channels this is defines the initial start
     * current.
     */
    uint16_t * reset_current;
	/** Pointer to the next bonded channel. */
	struct CN_DRV_Channel_Config_t * nextBondedChannel;
	/**
	 * Identifies if this channel responds to a DALI address.  1 means yes.
	 */
	CN_BOOL respondsToDali : 1;
	CN_BOOL respondsToDMX  : 1;

    /** Identifies if channel is bonded to another chanel or not */
    enum CN_DRV_Channel_Bond bonded : 1;
	uint16_t dmx_address : 9;
    /**
     * @brief A DALI Address if one is mapped.  Only valid if variable respondsToDali == 1.
     *
     * NOTE: This address is 0 based (0 - 63)
     *       This value should only be used if respondsToDali == 1.
     */
    uint8_t dali_address;

    /**
     * @brief Define the GW flash configuraton data
     */
    struct GW_Data
    {
        uint8_t expose_channel_to_gateway : 1;
        uint8_t expose_channel_as_group;
    }__attribute__((packed)) GW_Data;

} __attribute__((packed)) CN_DRV_Channel_Config_t;

typedef struct CN_DRV_Channel_State_t
{
    /** @brief last reported light level or DRIVER_INVALID_LEVEL if invalid */
    uint16_t level;
    /** @brief current channel faults - DRV_Channel_Faults */
    uint16_t fault_mask;

} CN_DRV_Channel_State_t;

typedef struct CN_DRV_Slot_t
{ // 68 bytes
	enum CN_DRV_Slot_Status         status : 4;
	/** @brief Configuration information status (exists, not exists) */
	enum CN_DRV_Slot_Config_Status  config_status : 1;

	/** @brief Driver Slot # (0 - 7) */
	uint8_t                         slotNumber : 3;
	enum CN_DRV_Type                driverType : 8;
	/** @brief Number of Drivers on the card in this slot ( 1 - DRIVER_MAX_CHANNELS_PER_DRIVER) */
	uint8_t                         driverCount : 4;
	/** @brief The firmware version of the driver */
	struct Firmware_Version_t       firmware_version;
	/** @brief The unique serial number of the driver */
	uint8_t                         serial_number[9];
	/** @brief model number of device */
	uint16_t                        model;
	/** @brief Array of one or more channel configurations - size should be same as [driverCount] */
	CN_DRV_Channel_Config_t         channelConfig[DRIVER__MAX_CHANNELS_PER_DRIVER];
	CN_DRV_Channel_State_t          channel_state[DRIVER__MAX_CHANNELS_PER_DRIVER];
	/** @brief The systick of the last time the channel state level was last updated */
	uint32_t                        channel_state_last_update_tick;
	/** @brief The firmware version of the driver's bootloader */
	struct Firmware_Version_t       bootloader_version;
	struct LM1_Driver_Metrics_t     last_driver_metrics;
	char                            git_commmit_hash[41];
	// amount of stack space remaining on the driver
	uint16_t                        stack_remaining;

	struct Firmware_Version_t       hardware_version;
	uint16_t                        product_id;
    CENSE_Logger_Level              logging_level;
    MCU_Reset_Reason                last_reset_reason;
    /** @brief stores the tick of when the last successful frame was received */
    uint32_t                        last_frame_received_tick;

} CN_DRV_Slot_t;

typedef struct CN_DRV_Slot_partition_01_t
{ 
	enum CN_DRV_Slot_Status         status : 4;
	enum CN_DRV_Slot_Config_Status  config_status : 1;
	uint8_t                         slotNumber : 3;
	enum CN_DRV_Type                driverType : 8;
	uint8_t                         driverCount : 4;
	struct Firmware_Version_t       firmware_version;
	uint8_t                         serial_number[9];
	uint16_t                        model;
	uint32_t                        channel_state_last_update_tick;
	struct Firmware_Version_t       bootloader_version;
	uint16_t                        stack_remaining;
	struct Firmware_Version_t       hardware_version;
    uint32_t                        last_frame_received_tick;
	uint16_t                        product_id;
    uint8_t                         logging_level;
    uint8_t                         last_reset_reason;
    
} CN_DRV_Slot_partition_01_t;

typedef struct CN_DRV_Slot_partition_02_t
{ 
	CN_DRV_Channel_Config_t         channelConfig[3];

} CN_DRV_Slot_partition_02_t;

typedef struct CN_DRV_Slot_partition_03_t
{ 
	CN_DRV_Channel_Config_t         channelConfig;
	CN_DRV_Channel_State_t          channel_state[DRIVER__MAX_CHANNELS_PER_DRIVER];


} CN_DRV_Slot_partition_03_t;

typedef struct CN_DRV_Slot_partition_04_t
{ 
	struct LM1_Driver_Metrics_t     last_driver_metrics;

} CN_DRV_Slot_partition_04_t;

typedef struct CN_DRV_Slot_partition_05_t
{ 
	char                            git_commmit_hash[41];

} CN_DRV_Slot_partition_05_t;

typedef enum CNGW_Config_Command
{
    CNGW_CONFIG_CMD_Invalid = 0x00,

    /**@brief Command used for CNGW_Channel_Entry_Message_t message.*/
    CNGW_CONFIG_CMD_Channel_Entry = 0x01,

    /**@brief Command used for CNGW_Channel_Status_Message_t message*/
    CNGW_CONFIG_CMD_Channel_Status = 0x02,

    CNGW_CONFIG_CMD_Config_Info_Driver = 0x03,
    CNGW_CONFIG_CMD_Config_Info_Wired_Switch = 0x04,
    CNGW_CONFIG_CMD_Config_Info_Wireless_Switch = 0x05,
    CNGW_CONFIG_CMD_Config_Info_Sensor = 0x06,
    CNGW_CONFIG_CMD_Config_General_Info = 0x07

} __attribute__((packed)) CNGW_Config_Command;

typedef enum CNGW_Config_Status
{
    CNGW_CONFIG_STATUS_Invalid = 0x00,

    /**@brief Sent from CN to GW to indicate all configuration were sent.
     * CN will send this message immediately after send the last
     * configuration
     *
     * This must also be sent if GW ask for the next message
     * but no more messages to be sent.*/
    CNGW_CONFIG_STATUS_Done = 0x01,

    /**@brief Sent from the GW, to notify the CN that it must send
     * the next configuration.
     * CN never send any data expect for status message
     * without GW first requesting it with this message.*/
    CNGW_CONFIG_STATUS_Get_Next_Msg = 0x02,

    /**@brief Sent from either CN or GW to restart or start
     * configuration transmission. This can occour
     * if something fails on either MCU.
     * HMAC is required when this status is setn from CN to GW only!*/
    CNGW_CONFIG_STATUS_Restart = 0x03,

    /**@brief Sent from CN when the exposed channel configuration
     * are overwritten in the external flash. CN does not need
     * to keep track of configuration.
     * CN just send this message if any of the data is overwritten.
     * GW will figured out if the current configuration match
     * it's last one or not.
     * Requires an HMAC*/
    CNGW_CONFIG_STATUS_Config_Change = 0x04,

} __attribute__((packed)) CNGW_Config_Status;

typedef struct CNGW_Channel_Status_Message_t
{
    CNGW_Config_Command command; /**@brief Set to CNGW_CONFIG_CMD_Channel_Status*/
    CNGW_Config_Status status;

    union
    {
        uint8_t crc;
        uint8_t hmac[CNGW_HMAC_LENGTH];
    } u;
} __attribute__((packed)) CNGW_Channel_Status_Message_t;

typedef struct CNGW_Channel_Configuration_Message_t
{
    CNGW_Config_Command command;  /**@brief Set to CNGW_CONFIG_CMD_Channel_Status*/
    uint8_t  slot;
    uint8_t frame_bytes[60];
    uint8_t crc;
} __attribute__((packed)) CNGW_Channel_Configuration_Message_t;

typedef struct CNGW_Channel_Configuration_Request_t
{
    CNGW_Config_Command command;  
    uint8_t  slot;
    uint8_t crc;
} __attribute__((packed)) CNGW_Channel_Configuration_Request_t;



typedef struct CNGW_Channel_Status_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Channel_Status_Message_t message;
} __attribute__((packed)) CNGW_Channel_Status_Frame_t;

typedef struct CNGW_Config_Message_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Channel_Configuration_Message_t message;
} __attribute__((packed)) CNGW_Config_Message_Frame_t;

typedef struct CNGW_Config_Request_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Channel_Configuration_Request_t message;
} __attribute__((packed)) CNGW_Config_Request_Frame_t;

struct total_configuration_t
{
    struct CN_Switch_Configuration_t wired_sw_config[WIRED_SWITCH_COUNT];
    struct CN_Switch_Configuration_t wireless_sw_config[WIRELESS_SWITCH_COUNT];
    CN_Sensor_Configuration_t sensor_config[MAX_SENSORS];
    CN_DRV_Slot_t driver[CABINET__DRIVER_SLOT_COUNT];

} __attribute__((packed));
typedef struct total_configuration_t * total_configuration_p;

#endif
#endif