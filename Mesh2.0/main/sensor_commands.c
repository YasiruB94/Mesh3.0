#ifdef IPNODE
#include "includes/sensor_commands.h"
static const char *TAG = "SensorCommands";

void Sensor_SendPendingMessage(EspNowStruct *recv_cb)
{
    char macStringMsg[14];
    strcpy(macStringMsg, recv_cb->macStr);
    strcat(macStringMsg, "m");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    size_t required_size;
    if (nvs_get_str(nvsHandle, macStringMsg, NULL, &required_size) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    if (required_size <= 1)
        return;
    ESP_LOGW(TAG, "This is the message size %d", (int)required_size);
    char messageForSensor[required_size];
    if (nvs_get_str(nvsHandle, macStringMsg, messageForSensor, &required_size) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    if (!EspNow_addMacAsPeer(recv_cb->mac_addr))
        return false;
    ESP_LOGW(TAG, "This is the message for Sensor %s", messageForSensor);
    esp_now_send(recv_cb->mac_addr, (uint8_t *)&messageForSensor, required_size);
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
    nvs_close(nvsHandle);
    esp_now_del_peer(recv_cb->mac_addr);
}

void Sensor_DeleteMessage(EspNowStruct *recv_cb)
{
    //Checking if this mac address is for this node or a group or another node
    char macStringMsg[14];
    strcpy(macStringMsg, recv_cb->macStr);
    // if (!Utilities_MacToStringShortVersion(recv_cb->mac_addr, macStringMsg, sizeof(macStringMsg))) return;
    strcat(macStringMsg, "m");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    if (nvs_erase_key(nvsHandle, macStringMsg) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    if (nvs_commit(nvsHandle) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
}

bool Sensor_AlreadyExists(EspNowStruct *recv_cb, size_t *required_size)
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    //size_t required_size;
    esp_err_t err = nvs_get_blob(nvsHandle, recv_cb->macStr, NULL, required_size);
    //Mac address dont exist
    if (err == ESP_ERR_NVS_NOT_FOUND && err != ESP_OK)
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
    //Sending the Sensor the information of this node e.g. mac address
    // We need to add a peer here to the ESP now peer list
    ESP_LOGW(TAG, "Sending friend info");
    if (!EspNow_addMacAsPeer(macOfSens))
        return false;
    cJSON *jFriendInfo = cJSON_CreateObject();
    cJSON_AddNumberToObject(jFriendInfo, "cmnd", 300);
    //char devMacStr[18];
    //Utilities_GetDeviceMac(devMacStr);
    cJSON_AddItemToObject(jFriendInfo, "val", cJSON_CreateString(deviceMACStr));
    char *dataToSend = cJSON_PrintUnformatted(jFriendInfo);
    esp_now_send(macOfSens, (uint8_t *)dataToSend, ((strlen(dataToSend) + 1) * sizeof(char)));
    cJSON_Delete(jFriendInfo);
    free(dataToSend);
    //Dont need to send anything to sensor, we can remove as peer for now
    esp_now_del_peer(macOfSens);
}
#endif
