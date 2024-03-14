#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_ACTION_H
#define CNGW_ACTION_H

#include "cngw_common.h"

typedef enum CNGW_Action_Command
{
    CNGW_ACTION_CMD_No_Action = 0x00,
    CNGW_ACTION_CMD_On_Full = 0x01,
    CNGW_ACTION_CMD_On_At_Level_X = 0x02,
    CNGW_ACTION_CMD_Off = 0x03,
    CNGW_ACTION_CMD_Execute_Scene = 0x04,
    CNGW_ACTION_CMD_Delay_Off = 0x0a,
    CNGW_ACTION_CMD_On_Last_Level = 0x0b,
    CNGW_ACTION_CMD_Up = 0x0c,
    CNGW_ACTION_CMD_Down = 0x0d,
    CNGW_ACTION_CMD_Toggle_On_Off = 0x0e

} __attribute__((packed)) CNGW_Action_Command;

typedef enum CNGW_Address_Type
{
    CNGW_ADDRESS_TYPE_Cense_Channel = 0x01,   /** Channel Number between 0 and 31 */
    CNGW_ADDRESS_TYPE_Cense_Group = 0x02,     /** Group Number between 1 and 255 */
    CNGW_ADDRESS_TYPE_Cense_Scene = 0x03,     /** Scene Number between 1 and 255 */
    CNGW_ADDRESS_TYPE_Cense_Broadcast = 0x0f, /** Broadcast all channels */
    CNGW_ADDRESS_TYPE_Dali_Address = 0x11,    /** DALI Short Address between 0 and 63 */
    CNGW_ADDRESS_TYPE_Dali_Group = 0x12,      /** DALI Group Address */
    CNGW_ADDRESS_TYPE_Dali_Broadcast = 0x1f   /** DALI Broadcast */
} __attribute__((packed)) CNGW_Address_Type;

typedef enum CNGW_Fade_Unit
{
    CNGW_FADE_UNIT_Milliseconds = 0x00,
    CNGW_FADE_UNIT_Seconds = 0x01,
    CNGW_FADE_UNIT_Minutes = 0x02
} __attribute__((packed)) CNGW_Fade_Unit;

/**
 * @brief Represents an address used as part of some messages
 */
typedef struct CNGW_Address_t
{
    uint8_t target_cabinet;
    CNGW_Address_Type address_type;
    uint8_t target_address;

} __attribute__((packed)) CNGW_Address_t;

/**
 * @brief Parameters structure for general action commands
 */
typedef struct CNGW_Action_Parameters_t
{
    CNGW_Fade_Unit fade_unit : 2;
    uint16_t fade_time : 11;   /** Fade time between 0 and 2047 */
    uint16_t light_level : 12; /** Light level between 0 and 2047 */
    uint8_t reserved : 7;
} __attribute__((packed)) CNGW_Action_Parameters_t;

/**
 * @brief Parameters structure for Execute Scene Action Command
 */
typedef struct CNGW_Action_Scene_Parameters_t
{
    uint32_t reserved;
} __attribute__((packed)) CNGW_Action_Scene_Parameters_t;

/**
 * @brief Represents an Action Message between gateway and control MCU
 *
 * All action commands share this same structure.
 *
 * @note This is send under the header type #CNGW_HEADER_TYPE_Action_Commmand
 *
 */
typedef struct CNGW_Action_Message_t
{
    CNGW_Action_Command command;
    CNGW_Address_t address;

    union
    {
        CNGW_Action_Parameters_t action_parameters;
        CNGW_Action_Scene_Parameters_t scene_parameters;
    };
    uint8_t crc;

} __attribute__((packed)) CNGW_Action_Message_t;

/**
 * @brief Action Message packet
 */
typedef struct CNGW_Action_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Action_Message_t message;

} __attribute__((packed)) CNGW_Action_Frame_t;

#endif
#endif