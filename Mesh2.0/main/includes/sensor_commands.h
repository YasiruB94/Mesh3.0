#ifdef IPNODE
#ifndef SENSOR_COMMANDS_H
#define SENSOR_COMMANDS_H

#include "esp_now_utilities.h"

extern void Sensor_SendPendingMessage(EspNowStruct *recv_cb);
extern void Sensor_DeleteMessage(EspNowStruct *recv_cb);
extern bool Sensor_AlreadyExists(EspNowStruct *recv_cb, size_t *required_size);
extern void Sensor_SendFriendInfo(const uint8_t *macOfSens);
#endif
#endif
