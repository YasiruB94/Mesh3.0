/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: cence_crc.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in calculating the CRC 8 value for data validity
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CENCE_CRC_H
#define CENCE_CRC_H

#include "includes/SpacrGateway_commands.h"
struct CRC_HandleTypeDef {
	uint32_t unused;
};
typedef struct CRC_HandleTypeDef CRC_HandleTypeDef;

/**
 * @brief Resets the CRC calculation
 *
 * @param hcrc CRC Handle
 *
 * @return The current value of the CRC from previous calls to #CENSE_CRC32_Accumulate
 */
uint32_t CENSE_CRC32_Reset(CRC_HandleTypeDef * hcrc);
/**
 * @brief Performs a CRC calculation on the supplied buffer.  This is an accumulation
 * 		  from previous calls to this function.  You should call @CENSE_CRC32_Reset() to
 * 		  start a new CRC calculation prior to calling this function
 *
 * @param hcrc CRC Handle
 * @param pBuffer[in]	input buffer to perform CRC calculation on
 * @param length			size of the input buffer
 * @return
 */
uint32_t CENSE_CRC32_Accumulate(uint32_t * pBuffer, uint32_t length);

uint32_t calculate_total_crc(const uint8_t * initPtr, size_t total_size);


#endif
#endif