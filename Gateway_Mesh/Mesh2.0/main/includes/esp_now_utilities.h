#ifdef IPNODE
#ifndef ESP_NOW_UTILITIES_H
#define ESP_NOW_UTILITIES_H

#include "utilities.h"
#include "node_utilities.h"

extern QueueHandle_t espNowQueue;
extern char *sensorSecret;
// extern const char *PMK_KEY_STR;
// extern const char *LMK_KEY_STR;

typedef struct
{
    char sensorSecret[8];
    uint8_t devType;
    char devID[18];
    char meshID[MESH_ID_LENGTH];
    float volt;
    uint16_t cmnd;
    uint8_t registerDev;
    float val;
    char str[20];
    uint8_t mode;
    uint16_t crc;
} integStruct;

typedef struct
{
    integStruct structCheck;
    uint8_t mState;

} motionStruct;

typedef struct
{
    integStruct structCheck;
    double flowSpeed;

} flowStruct;

typedef struct
{
    integStruct structCheck;
    float temperature;
    float humidity;
    float pressure;
    float co2;
    float VOC;
    float airquality;
    uint16_t illuminance;
    uint16_t colortemperature;
    uint8_t accuracy;
} ieqStruct;

typedef struct
{
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    char shortDevMac[13];
    uint8_t *data;
    uint8_t devType;
    char devID[18];
    char meshID[MESH_ID_LENGTH];
    uint8_t mode;
    float volt;
    uint16_t cmnd;
    uint8_t registerDev;
    float val;
    char str[20];
} EspNowStruct;

typedef struct
{
    uint16_t cmnd;
    float val;
    char str[50]; // Cannot use string pointers sending to ESP-NOW
    char sensorSecret[8];
    uint16_t crc;

} sensorDataStruct;

extern bool EspNow_addMacAsPeer(const uint8_t *mac);
extern bool EspNow_CheckAction(EspNowStruct *sensorDataRecv);
extern void EspNow_RecvCb(const uint8_t *mac_addr, const uint8_t *data, int len);
extern void EspNow_SendCb(const uint8_t *mac_addr, esp_now_send_status_t status);
extern void EspNow_CreateEspNowQueue();
extern void EspNow_Init();

#endif
#endif