#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ****************************************************************************
 * --- Global Defines ---
 ******************************************************************************/
static const uint8_t data_ZEROS[32] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                                        , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const uint8_t data_0[36] = { 0xa2, 0x2f, 0x58, 0x43, 0x36, 0x48, 0x1d, 0x5e, 0xa1, 0xed, 0x12, 0x24, 0x97, 0xc0, 0x02, 0x86
                                                        , 0x74, 0x83, 0x5d, 0x51, 0x60, 0x50, 0xec, 0x24, 0x78, 0xec, 0xa7, 0x34, 0xc7, 0x08, 0x9c, 0xe9
                                                        , 0x00, 0x00, 0x00, 0x00 };
static const uint8_t data_2[36] = { 0xca, 0x55, 0xac, 0xe5, 0x73, 0x57, 0xa1, 0x14, 0x2d, 0x86, 0x8f, 0x83, 0x37, 0x9f, 0x5c, 0x0f
                                                        , 0xbc, 0xa0, 0xd0, 0xd8, 0x3a, 0xe0, 0x8c, 0x06, 0x27, 0xcb, 0x1e, 0xcb, 0xc7, 0x07, 0xbc, 0xbc
                                                        , 0x00, 0x00, 0x00, 0x00 };
static const uint8_t data_3[36] = { 0x13, 0x4c, 0x88, 0x1e, 0x0f, 0x86, 0x8d, 0x35, 0x0e, 0x5f, 0xe0, 0x00, 0xd2, 0xda, 0x49, 0x48
                                                        , 0xd5, 0xa1, 0x3f, 0xef, 0x00, 0x8f, 0x83, 0x53, 0x3c, 0x6a, 0xab, 0x4f, 0x13, 0x3a, 0xa1, 0x15
                                                        , 0x00, 0x00, 0x00, 0x00 };

static const uint8_t data_9[96] = { 0x00, 0x00, 0x00, 0x00
                                                , 0xb4, 0xe1, 0x5c, 0x06, 0xce, 0x63, 0x54, 0x79, 0xae, 0xb4, 0x06, 0xea, 0x9c, 0xc1, 0x7d, 0x51
                                                , 0xbb, 0xb6, 0x73, 0x82, 0x4f, 0x7f, 0x5b, 0x94, 0x66, 0xe1, 0x37, 0xe7, 0xb7, 0x8e, 0xc2, 0xfd
                                                , 0x00, 0x00, 0x00, 0x00
                                                , 0x29, 0x77, 0xf3, 0x5f, 0x84, 0xdd, 0x15, 0xf0, 0x63, 0x49, 0x91, 0x50, 0xc7, 0xf1, 0x72, 0xeb
                                                , 0x62, 0xe4, 0x7b, 0x4a, 0x8a, 0x4c, 0x87, 0x29, 0x11, 0x1b, 0x2a, 0x1f, 0x47, 0x11, 0x8f, 0x66
                                                , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                                , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


#ifdef __cplusplus
}
#endif
#endif