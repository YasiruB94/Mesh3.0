#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef AT508_STRUCTS_H
#define AT508_STRUCTS_H
#include "mbedtls/sha256.h"

typedef enum
{
    ATECC508A_ZONE_CONFIG   = 0x00,
    ATECC508A_ZONE_OTP      = 0x01,
    ATECC508A_ZONE_DATA     = 0x02,
} atecc508a_zone_t;

typedef enum
{
    ATECC508A_ADDRESS_CONFIG_READ_BLOCK_0 = 0x0000,
    ATECC508A_ADDRESS_CONFIG_READ_BLOCK_1 = 0x0008,
    ATECC508A_ADDRESS_CONFIG_READ_BLOCK_2 = 0x0010,
    ATECC508A_ADDRESS_CONFIG_READ_BLOCK_3 = 0x0018,
} atecc508a_addr_t;

typedef struct
{
    uint16_t crc;
} atecc508a_crc_ctx_t;

/**
 * @brief The Context for @ref ATMEL_Read_Data()
 * and @ref ATMEL_Write_Data()
 */
struct ATMEL_RW_CONTEXT
{
    uint8_t         zone;   /**The zone to read/write at*/
    uint8_t         slot;   /**The zone to read/write at*/
    uint8_t         block;  /**The block to read/write at*/
    uint8_t         offset; /**The offset to read at*/
    uint8_t         *data;   /**Buffer to copy to data into*/
    uint8_t         len;    /**Length of the buffer between 4 and 32*/
};
typedef struct ATMEL_RW_CONTEXT ATMEL_RW_CTX_t;

/**
 * @brief The original ATCA packet struct
 * from the ATCA library.
 * This structure is sent over the wrire
 */
typedef struct ATCAPACKET_t
{
    uint8_t     word_address;  /**HAL layer as needed (I/O tokens, Word address values)*/

    /**--- start of packet i/o frame----*/
    uint8_t     txsize;
    uint8_t     opcode;
    uint8_t     param1;
    uint16_t    param2;

    /** includes 2-byte CRC.  data size is determined by largest possible data section of any
     command + crc (see: x08 verify data1 + data2 + data3 + data4)
     this is an explicit design trade-off (space) resulting in simplicity in use
     and implementation*/
    uint8_t     data[130];
    /**--- end of packet i/o frame ---*/

    /**--- Used for receiving ---*/
    /*uint8_t execTime;*/
    uint16_t rxsize;        /**expected response size, response is held in data member*/

} __attribute__((packed)) ATCAPACKET_t;

#endif
#endif