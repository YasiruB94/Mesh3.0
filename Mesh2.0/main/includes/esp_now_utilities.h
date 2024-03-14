#ifdef IPNODE
#ifndef ESP_NOW_UTILITIES_H
#define ESP_NOW_UTILITIES_H

#include "utilities.h"
#include "node_utilities.h"

extern QueueHandle_t espNowQueue;

typedef struct
{
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    char *macStr;
    uint8_t *data;
    int data_len;
} EspNowStruct;

extern bool EspNow_addMacAsPeer(const uint8_t *mac);
extern void EspNow_ParseData(EspNowStruct *recv_cb, size_t *required_size, uint8_t *sensorAction, char *macOfNodeStr);
extern void EspNow_RecvCb(const uint8_t *mac_addr, const uint8_t *data, int len);
extern void EspNow_SendCb(const uint8_t *mac_addr, esp_now_send_status_t status);
extern void EspNow_CreateEspNowQueue();
extern void EspNow_Init();

#endif
#endif