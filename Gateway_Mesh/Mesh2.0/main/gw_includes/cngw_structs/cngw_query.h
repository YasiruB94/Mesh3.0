#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#ifndef CNGW_QUERY_H
#define CNGW_QUERY_H

#include "cngw_common.h"

typedef enum CNGW_Query_Command
{
	/** @brief Sent from CN to GW.
	 * Signal when CN is ready to receive backward data from GW*/
	CNGW_QUERY_CMD_Backward_Frame = 0x02,

	/**@brief Sent from GW to CN.
	 * Used when GW wants CN to send it all the channel status.
	 * CN responds with CNGW_Attribute_Update_Message_t for
	 * each channel
	 * It is perfectly save for CN to send address which are not
	 * exposed to the GW. GW will ignore them.*/
	CNGW_QUERY_CMD_Get_All_Channel_Info = 0x03
} __attribute__((packed)) CNGW_Query_Command;

typedef struct CNGW_Query_Message_t
{
	CNGW_Query_Command command; /** @brief A command from CNGW_Query_Command */
	uint8_t crc;				/** @brief CRC-8 of the message */
} __attribute__((packed)) CNGW_Query_Message_t;

typedef struct CNGW_Query_Message_Frame_t
{
	CNGW_Message_Header_t header;
	CNGW_Query_Message_t message;
} __attribute__((packed)) CNGW_Query_Message_Frame_t;

#endif
#endif