/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: cence_crc.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in calculating the CRC 8 value for data validity
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/cence_crc.h"
static const char *TAG = "cence_crc";

static uint32_t crc = 0xFFFFFFFF;

/**
 * @brief Resets the CRC calculation
 *
 * @param hcrc CRC Handle
 *
 * @return The current value of the CRC from previous calls to #CENSE_CRC32_Accumulate
 */
uint32_t CENSE_CRC32_Reset(CRC_HandleTypeDef * hcrc) {
	uint32_t current_crc = crc;

	crc = 0xFFFFFFFF;

	return current_crc;
}


/**
 * @brief Performs a CRC calculation on the supplied buffer.  This is an accumulation
 * 		  from previous calls to this function.  You should call @CENSE_CRC32_Reset() to
 * 		  start a new CRC calculation prior to calling this function
 *
 * 		  This implementation matches the STM32 hardware CRC in 32bit word mode.
 *
 * @param hcrc CRC Handle
 * @param pBuffer[in]	input buffer to perform CRC calculation on
 * @param length			size of the input buffer
 * @return
 */
uint32_t CENSE_CRC32_Accumulate(uint32_t * pBuffer, uint32_t length) {

	  for (uint32_t word = 0; word < length; word++) {
		  crc = crc ^ pBuffer[word];

		  for(uint32_t i = 32; i--;)
			if (crc & 0x80000000)
				crc = (crc << 1) ^ 0x04C11DB7; // Polynomial used in STM32
			else
				crc = (crc << 1);
	  }
	  return crc;
}

uint32_t calculate_total_crc(const uint8_t *initPtr, size_t total_size)
{
    size_t remaining = total_size;
    const uint8_t *ota_agent_core_current_ptr;
    uint16_t BLOCK_SIZE = 128U;
    ota_agent_core_current_ptr = initPtr;
    uint16_t ota_agent_core_block_count = 0;
    crc = 0xFFFFFFFF;
    while (remaining > 0U)
    {
        // 1. initialize and set function specific variables
        uint8_t binary[BLOCK_SIZE];
        memset(binary, 0, sizeof(binary));

        // 2. build the header
        const int32_t write_bytes = MIN((int32_t)BLOCK_SIZE, remaining);

        // 3. build the message
        void *read_buffer = malloc(write_bytes);
        if (read_buffer == NULL)
        {
            // failed to allocate malloc memory. exit loop
            ESP_LOGE(TAG, "Failed to allocate memory");
        }
        else
        {
            // read OTA data from required packet
            esp_err_t result = spi_flash_read((uint32_t)ota_agent_core_current_ptr, read_buffer, write_bytes);
            if (result == ESP_OK)
            {
                if (ota_agent_core_block_count % 100 == 0)
                {
                    ESP_LOGI(TAG, "block count: %d, remaining bytes: %d", ota_agent_core_block_count, remaining);
                }

                CENSE_CRC32_Accumulate((uint32_t *)read_buffer, (write_bytes >> 2U));
            }
            else
            {
                // failed to read SPI flash memory. exit loop
                ESP_LOGE(TAG, "SPI failed to read: %s", esp_err_to_name(result));
            }
        }

        free(read_buffer);
        // 4. update the pointer for the next loop
        ota_agent_core_current_ptr += write_bytes;
        remaining -= write_bytes;
        ota_agent_core_block_count++;
    }

    return crc;
}

#endif