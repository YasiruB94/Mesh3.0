#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CNGW_MISCELLANEOUS_H
#define CNGW_MISCELLANEOUS_H

#include "../cn_info_structs/general.h"
#include "cngw_common.h"
#include "cngw_ota.h"
#define NUM_DR_DEVICES 8
#define NUM_DR_CHANNELS 32
/**
 * @brief A message holding all the device information
 *        for one MCU.
 *        CN send this to GW.
 */
typedef struct CNGW_Device_Info_Update_Message_t
{
    CNGW_Device_Info_Command command;
    CNGW_Source_MCU mcu;
    uint8_t serial[CNGW_SERIAL_NUMBER_LENGTH];
    uint8_t model;
    uint8_t git_hash[20];
    CNGW_Hardware_Version_t hardware_version;
    CNGW_Firmware_Version_t bootloader_version;
    CNGW_Firmware_Version_t application_version;
    uint8_t crc;
} __attribute__((packed)) CNGW_Device_Info_Update_Message_t;

typedef struct CN_General_Config_t
{
    uint8_t serial_id[9];
    uint8_t dali_ps_active;
    uint8_t slot_count;
    uint8_t wired_switch_count;
    uint8_t wireless_switch_count;
    uint16_t cabinet_number;
    uint8_t volatile flash_programmed;
    CNGW_Firmware_Version_t sw_version;
    CNGW_Firmware_Version_t sw_bootloader_version;
    char sw_git_commit[41];
    bool sw_is_handshake; /*Set when the SW handshake at least once*/
    bool cn_is_handshake;
} __attribute__((packed)) CN_General_Config_t;

typedef struct CN_General_Config_Reduced_t {
    uint8_t                     serial_id[9];
    uint8_t                     dali_ps_active;
    uint8_t                     slot_count;
    uint8_t                     wired_switch_count;
    uint8_t                     wireless_switch_count;
    uint16_t                    cabinet_number;
    uint8_t volatile            flash_programmed;
    CNGW_Firmware_Version_t     sw_version;
    CNGW_Firmware_Version_t     sw_bootloader_version;
    CN_BOOL                     sw_is_handshake; /*Set when the SW handshake at least once*/
} CN_General_Config_Reduced_t;



/**
 * @brief A message indicate the MCU has been removed
 *        CN send this to GW.
 *
 *        When CN notice a driver has been removed
 *        it should wait reasonable time
 *        because a user could be plugging in another
 *        driver in a short amount time.
 *        This should be around 30 second to 1 minute.
 */
typedef struct CNGW_Device_Info_Remove_Message_t
{
    CNGW_Device_Info_Command command;
    CNGW_Source_MCU mcu; /**@brief The driver MCU that was removed. Can only be drivers. */
    uint8_t crc;
} __attribute__((packed)) CNGW_Device_Info_Remove_Message_t;

/**
 * @brief An attribute update message for a attribute
 *        per channel.
 */
typedef struct CNGW_Update_Attribute_Message_t
{
    CNGW_Update_Command command_type; /**@brief Always set to CNGW_UPDATE_CMD_Attribute*/
    CNGW_Address_t address;
    CNGW_Channel_Attribute_Type attribute;
    uint16_t value;
    uint8_t crc;
} __attribute__((packed)) CNGW_Update_Attribute_Message_t;

/**
 * @brief An attribute update message for a attribute
 *        per channel.
 */
typedef struct CNGW_Update_Channel_Status_Message_t
{
    CNGW_Update_Command command_type; /**@brief Always set to CNGW_UPDATE_CMD_Status*/
    CNGW_Address_t address;

    /**@brief Bit mask of the status using CNGW_Update_Channel_Status. Value of 0 indicate no faults.
     * The Gateway will always directly write out this value to it's database.
     * It does not bit mask it. Thus CN must always send the entire mask each time.*/
    uint32_t status_mask;
    uint8_t crc;
} __attribute__((packed)) CNGW_Update_Channel_Status_Message_t;

/**
 * @brief A frame describing CNGW_Device_Info_Message_t
 *        Header command is CNGW_HEADER_TYPE_Device_Report
 */
typedef struct CNGW_Device_Info_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Device_Info_Update_Message_t message;
} __attribute__((packed)) CNGW_Device_Info_Frame_t;

/**
 * @brief A frame describing CNGW_Device_Info_Remove_Message_t
 *        Header command is CNGW_HEADER_TYPE_Device_Report
 */
typedef struct CNGW_Device_Info_Remove_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Device_Info_Remove_Message_t message;
} __attribute__((packed)) CNGW_Device_Info_Remove_Frame_t;

/**
 * @brief The frame for CNGW_Update_Attribute_Message_t
 */
typedef struct CNGW_Update_Attribute_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Update_Attribute_Message_t message;
} __attribute__((packed)) CNGW_Update_Attribute_Frame_t;

/**
 * @brief The frame for CNGW_Update_Channel_Status_Message_t
 */
typedef struct CNGW_Update_Channel_Status_Frame_t
{
    CNGW_Message_Header_t header;
    CNGW_Update_Channel_Status_Message_t message;
} __attribute__((packed)) CNGW_Update_Channel_Status_Frame_t;

typedef struct Binary_Data_Pkg_Dist_Release_Ver_t
{
    uint8_t major : 8;
    uint8_t minor : 8;
    uint32_t ci : 29;
    uint8_t branch_id : 3;

} __attribute__((packed)) Binary_Data_Pkg_Dist_Release_Ver_t;

typedef struct Binary_Data_Pkg_Info_t
{
    const char *binary_start;
    const char *binary_end;
    size_t binary_size;
    size_t binary_size_mod;
    const uint8_t *initial_ptr;
    Binary_Data_Pkg_Dist_Release_Ver_t dist_release_ver;
    CNGW_Firmware_Binary_Type binary_type;
    Binary_Data_Pkg_Dist_Release_Ver_t binary_ver;
    uint32_t binary_full_crc;

} __attribute__((packed)) Binary_Data_Pkg_Info_t;

typedef struct CNGW_CN_Board_Info_t
{
    CNGW_Device_Info_Update_Message_t cn_mcu;
    CNGW_Device_Info_Update_Message_t sw_mcu;
    CNGW_Device_Info_Update_Message_t gw_mcu;
    CNGW_Device_Info_Update_Message_t dr_mcu[NUM_DR_DEVICES];
    CNGW_Update_Attribute_Message_t channel_attribute[NUM_DR_CHANNELS];
    CNGW_Update_Channel_Status_Message_t channel_status[NUM_DR_CHANNELS];
    CN_General_Config_t cn_config;

} __attribute__((packed)) CNGW_CN_Board_Info_t;

/**
 * @brief The command for each message
 */
typedef enum CNGW_AWS_Command
{
  CNGW_AWS_CMD_UPDATE_CN = 0x00,
  CNGW_AWS_CMD_UPDATE_SW = 0x01,
  CNGW_AWS_CMD_UPDATE_GW = 0x02,
  CNGW_AWS_CMD_UPDATE_DR = 0x03,
  CNGW_AWS_CMD_Attribute = 0x04,
  CNGW_AWS_CMD_Status = 0x05,
  CNGW_AWS_CMD_Gen_Config = 0x06,

} __attribute__((packed)) CNGW_AWS_Command;



typedef struct CNGW_AWS_Response_t
{
    uint8_t CN_Serial[CNGW_SERIAL_NUMBER_LENGTH];
    CNGW_AWS_Command message_type;
    union
    {
        CNGW_Device_Info_Update_Message_t info_update;
        CNGW_Update_Attribute_Message_t attribute;
        CNGW_Update_Channel_Status_Message_t channel_status;
        CN_General_Config_t gen_config;
    };

} __attribute__((packed)) CNGW_AWS_Response_t;

#endif
#endif