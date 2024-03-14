#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef CN_GENERAL_H
#define CN_GENERAL_H

typedef enum CN_BOOL
{
    CN_FALSE    = 0,
    CN_TRUE     = 1
} CN_BOOL;

typedef struct Firmware_Version_t  {
	uint8_t 	 major_version 	: 8;
	uint8_t  minor_version 	: 8;
	uint32_t ci_build_number	: 29;
	uint8_t 	 branch_id 		: 3;
} __attribute__((packed)) Firmware_Version_t;



#endif
#endif