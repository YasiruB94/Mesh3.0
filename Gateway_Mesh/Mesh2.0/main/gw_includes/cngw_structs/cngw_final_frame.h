#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_FINAL_FRAME_H
#define CNGW_FINAL_FRAME_H

#include "cngw_common.h"
#include "cngw_action.h"
#include "cngw_config.h"
#include "cngw_control.h"
#include "cngw_direct_control.h"
#include "cngw_handshake.h"
#include "cngw_log.h"
#include "cngw_miscellaneous.h"
#include "cngw_ota.h"
#include "cngw_query.h"

/**
 * @brief The TX frames which can be sent to CN
 */
union CCP_TX_FRAMES
{
    /*Frame which are supported*/
    CNGW_Action_Frame_t *action;
    CNGW_Ota_Info_Frame_t *ota_info;
    CNGW_Ota_Binary_Data_Frame_t *ota_binary;
    CNGW_Channel_Status_Frame_t *channel_status;
    CNGW_Query_Message_Frame_t *query;
    CNGW_Handshake_GW1_Frame_t *gw1;
    CNGW_Control_Frame_t *control_commands;
    CNGW_Direct_Control_Frame_t *direct_control_commands;

    /*Access only pointer for ease of use and strictness*/
    const void *const raw_data;
    const CNGW_Message_Header_t *const generic_header;
};
typedef union CCP_TX_FRAMES CCP_TX_FRAMES_t;

#endif
#endif