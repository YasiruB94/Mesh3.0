#ifdef IPNODE
#include "includes/sensor_commands.h"
static const char *TAG = "SensorCommands";

void Sensor_SendPendingMessage(EspNowStruct *sensorDataRecv)
{
    char macStringMsg[14];
    strcpy(macStringMsg, sensorDataRecv->shortDevMac);
    strcat(macStringMsg, "m");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    size_t required_size;
    if (nvs_get_blob(nvsHandle, macStringMsg, NULL, &required_size) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    ESP_LOGW(TAG, "This is the message size %d", (int)required_size);
    if (required_size <= 1 || required_size != sizeof(sensorDataStruct))
    {
        nvs_close(nvsHandle);
        return;
    }
    uint8_t messageForSensor[required_size];
    if (nvs_get_blob(nvsHandle, macStringMsg, messageForSensor, &required_size) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    if (!EspNow_addMacAsPeer(sensorDataRecv->mac_addr))
        return false;
    // ESP_LOGW(TAG, "This is the message for Sensor %s", messageForSensor);
    esp_err_t ret = espnow_send(ESPNOW_TYPE_DATA, sensorDataRecv->mac_addr, &messageForSensor, required_size, NULL, portMAX_DELAY);

    // if message is for update, trigger sending of update
    sensorDataStruct *sensorData = (sensorDataStruct *)messageForSensor;
    if (sensorData->cmnd == 304)
    {
        ESP_LOGI(TAG, "trigger sensor update");
        NodeUtilities_SendFirmware(firmware_size, sha_256, sensorDataRecv->mac_addr);
    }
    if (ret == ESP_OK)
    {
        if (nvs_erase_key(nvsHandle, macStringMsg) != ESP_OK)
        {
            nvs_close(nvsHandle);
            return;
        }
        if (nvs_commit(nvsHandle) != ESP_OK)
        {
            nvs_close(nvsHandle);
            return;
        }
    }
    nvs_close(nvsHandle);

    ret = espnow_del_peer(sensorDataRecv->mac_addr);
    if (ret != ESP_OK)
    {
        ESP_LOGI(TAG, "Deleting peer unsuccessful");
        return;
    }
}

bool Sensor_AlreadyExists(EspNowStruct *sensorDataRecv)
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, sensorDataRecv->shortDevMac, NULL, &required_size);
    // Mac address dont exist
    if (err == ESP_ERR_NVS_NOT_FOUND || err != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGW(TAG, "Sensor Does not exist");
        return false;
    }
    nvs_close(nvsHandle);
    return true;
}

void Sensor_SendFriendInfo(const uint8_t *macOfSens)
{
    // Sending the Sensor the information of this node e.g. mac address
    //  We need to add a peer here to the ESP now peer list
    ESP_LOGW(TAG, "Sending friend info");
    if (!EspNow_addMacAsPeer(macOfSens))
        return false;
    sensorDataStruct dataToSendSensor;
    dataToSendSensor.cmnd = 300;
    dataToSendSensor.val = 0;
    memset(dataToSendSensor.str, '\0', sizeof(dataToSendSensor.str));
    strcpy(dataToSendSensor.str, deviceMACStr);
    strcpy(dataToSendSensor.sensorSecret, sensorSecret);
    dataToSendSensor.crc = 0;
    dataToSendSensor.crc = crc16_le(UINT16_MAX, (uint8_t const *)&dataToSendSensor, sizeof(sensorDataStruct));
    // esp_now_send(macOfSens, (uint8_t *)&dataToSendSensor, sizeof(dataToSendSensor));
    espnow_send(ESPNOW_TYPE_DATA, macOfSens, &dataToSendSensor, sizeof(sensorDataStruct), NULL, portMAX_DELAY);
    // Dont need to send anything to sensor, we can remove as peer for now
    esp_err_t ret = espnow_del_peer(macOfSens);
    if (ret != ESP_OK)
    {
        ESP_LOGI(TAG, "Deleting peer unsuccessful");
        return;
    }
}

char *Sensor_CastStructToCjson(EspNowStruct *sensorDataRecv)
{
    cJSON *jsonObj = cJSON_CreateObject();
    cJSON_AddItemToObject(jsonObj, "devID", cJSON_CreateString(sensorDataRecv->devID));
    cJSON_AddNumberToObject(jsonObj, "devType", sensorDataRecv->devType);
    cJSON_AddNumberToObject(jsonObj, "volt", sensorDataRecv->volt);
    cJSON_AddNumberToObject(jsonObj, "cmnd", sensorDataRecv->cmnd);
    cJSON_AddNumberToObject(jsonObj, "val", sensorDataRecv->val);
    cJSON_AddItemToObject(jsonObj, "str", cJSON_CreateString(sensorDataRecv->str));
    cJSON_AddItemToObject(jsonObj, "meshID", cJSON_CreateString(sensorDataRecv->meshID));
    cJSON_AddItemToObject(jsonObj, "peerNode", cJSON_CreateString(deviceMACStr));
    switch (sensorDataRecv->devType)
    {
    case 4:
    { // motion Sensor
        motionStruct *motionSensor = (motionStruct *)sensorDataRecv->data;
        cJSON_AddNumberToObject(jsonObj, "mState", motionSensor->mState);
        break;
    }
    case 5:
    {
        // IEQ goes here
        ieqStruct *ieqStructData = (ieqStruct *)sensorDataRecv->data;
        cJSON_AddNumberToObject(jsonObj, "temperature", ieqStructData->temperature);
        cJSON_AddNumberToObject(jsonObj, "humidity", ieqStructData->humidity);
        cJSON_AddNumberToObject(jsonObj, "pressure", ieqStructData->pressure);
        cJSON_AddNumberToObject(jsonObj, "co2", ieqStructData->co2);
        cJSON_AddNumberToObject(jsonObj, "airquality", ieqStructData->airquality);
        cJSON_AddNumberToObject(jsonObj, "VOC", ieqStructData->VOC);
        cJSON_AddNumberToObject(jsonObj, "accuracy", ieqStructData->accuracy);
        cJSON_AddNumberToObject(jsonObj, "illuminance", ieqStructData->illuminance);
        cJSON_AddNumberToObject(jsonObj, "colortemperature", ieqStructData->colortemperature);
        break;
    }
    case 9:
    {
        flowStruct *flowSensor = (flowStruct *)sensorDataRecv->data;
        cJSON_AddNumberToObject(jsonObj, "flowSpeed", flowSensor->flowSpeed);
    }
    }
    // ESP_LOGI(TAG, "Data to Send sensor %s", sensorDataToSend);
    char *printSensorData = cJSON_PrintUnformatted(jsonObj);
    cJSON_Delete(jsonObj);
    return printSensorData;
}

bool Sensor_SendCommandToSensor(char *sensorMac, sensorDataStruct *command)
{
    nvs_handle nvsHandle;
    char sensorMacM[14];
    if (sensorMac == NULL || strlen(sensorMac) != 12)
    {
        return false;
    }
    strcpy(sensorMacM, sensorMac);
    strcat(sensorMacM, "m");

    // set sensor command in NVS
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    esp_err_t err = nvs_set_blob(nvsHandle, sensorMacM, command, sizeof(sensorDataStruct));
    if (err != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    if (nvs_commit(nvsHandle) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
    ESP_LOGI(TAG, "command queued for sensor");
    return true;
}
#endif
