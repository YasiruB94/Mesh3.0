#ifdef IPNODE
#ifndef SENSOR_COMMANDS_H
#define SENSOR_COMMANDS_H

#include "esp_now_utilities.h"

extern void Sensor_SendPendingMessage(EspNowStruct *sensorDataRecv);
extern bool Sensor_AlreadyExists(EspNowStruct *sensorDataRecv);
extern void Sensor_SendFriendInfo(const uint8_t *macOfSens);
extern char *Sensor_CastStructToCjson(EspNowStruct *sensorDataRecv);
extern bool Sensor_SendCommandToSensor(char *sensorMac, sensorDataStruct *command);
#endif
#endif
