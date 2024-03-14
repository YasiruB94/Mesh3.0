#ifdef IPNODE
#include "includes/esp_now_utilities.h"
static const char *TAG = "EspNowUtilities";
// const char *PMK_KEY_STR = "PRIVATEMASTERKEY";
// const char *LMK_KEY_STR = "LOCAL_MASTER_KEY";

QueueHandle_t espNowQueue;
char *sensorSecret = SENSOR_SECRET;

void EspNow_CreateEspNowQueue()
{
    // created a queue with maxumum of 15 commands, and a item size of a char pointer
    espNowQueue = xQueueCreate(30, sizeof(EspNowStruct *));
    if (espNowQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create ESP NOW queue");
        // need to restart and let the backend know
    }
}

bool EspNow_addMacAsPeer(const uint8_t *mac)
{
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        return false;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    peer->ifidx = ESP_IF_WIFI_STA;
    peer->encrypt = false;
    memcpy(peer->peer_addr, mac, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(peer);
    free(peer);
    return true;
}

char *EspNow_CheckAction_Util(EspNowStruct *sensorDataRecv)
{
    // Here right now we are going to form cJSON, but when nodeoperations converts to structs, we can just send a struct!
    cJSON *jsonObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(jsonObj, "cmnd", sensorDataRecv->cmnd);
    cJSON_AddNumberToObject(jsonObj, "val", sensorDataRecv->val);
    cJSON_AddItemToObject(jsonObj, "str", cJSON_CreateString(sensorDataRecv->str));
    // Added usrID for sensor control data in order to store this into timestream!
    cJSON_AddItemToObject(jsonObj, "usrID", cJSON_CreateString("sensor"));
    char *sensorDataToSend = cJSON_PrintUnformatted(jsonObj);
    char *sensorDataToSendPv = (char *)pvPortMalloc((sizeof(char) * (strlen(sensorDataToSend) + 1)));
    strcpy(sensorDataToSendPv, sensorDataToSend);
    free(sensorDataToSend);
    cJSON_Delete(jsonObj);
    return sensorDataToSendPv;
}

bool EspNow_CheckAction(EspNowStruct *sensorDataRecv)
{
    ESP_LOGW(TAG, "Parsing ESP NOW data");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
    {
        return false;
    }
    size_t required_size = 0;
    esp_err_t foundBlob = nvs_get_blob(nvsHandle, sensorDataRecv->shortDevMac, NULL, &required_size);
    // IF error OR it cannot find the key, we assume that its either on auto pairing mode which means that theres no key
    if (foundBlob == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(nvsHandle);
        return true;
    }
    else if (foundBlob != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }

    // if key is found and required size is 1, we can automatically assume its an action = 1;
    if (required_size == 1)
    {
        uint8_t sensorAction = DEFAULT_SENSOR_ACTION;
        if (nvs_get_blob(nvsHandle, sensorDataRecv->shortDevMac, &sensorAction, &required_size) != ESP_OK)
        {
            nvs_close(nvsHandle);
            return false;
        }
        if (sensorAction == 1)
        {
            char *sensorDataToSendPv = EspNow_CheckAction_Util(sensorDataRecv);
            // We going to send data to node here to control lights or whatever else needed
            if (xQueueSendToBack(nodeReadQueue, &sensorDataToSendPv, (TickType_t)0) != pdPASS)
            {
                ESP_LOGE(TAG, "Queue is full");
                vPortFree(sensorDataToSendPv);
                // Report back to Root here letting us know that the queue is full and to wait for the next command
                // The queue most likely will neveer fill up but you never know
            }
        }
        nvs_close(nvsHandle);
        return true;
    }
    // If size is bigger than it's probably send to a node or a group
    else if (required_size > 1)
    {
        char *sensorDataToSendPv = EspNow_CheckAction_Util(sensorDataRecv);
        ESP_LOGW(TAG, "Sensor data is for group or node");
        uint8_t macForNodeOrGroup[required_size];
        if (nvs_get_blob(nvsHandle, sensorDataRecv->shortDevMac, &macForNodeOrGroup, &required_size) != ESP_OK)
        {
            nvs_close(nvsHandle);
            vPortFree(sensorDataToSendPv);
            return false;
        }
        if (NodeUtilities_compareWithGroupMac(macForNodeOrGroup))
        {
            NodeUtilities_SendDataToGroup(macForNodeOrGroup, sensorDataToSendPv);
        }
        else
        {
            ESP_LOGW(TAG, "Sensor data is sending to node");
            NodeUtilities_SendToNode(macForNodeOrGroup, sensorDataToSendPv);
        }
        vPortFree(sensorDataToSendPv);
    }
    nvs_close(nvsHandle);
    return true;
}

void EspNow_RecvCb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    // if (mac_addr == NULL || data == NULL || len < 5)
    // {
    //     ESP_LOGW(TAG, "Received Bad data from ESP-NOW");
    //     return;
    // }

    integStruct *integDataCheck = (integStruct *)data;
    uint16_t crc, crc_cal = 0;
    crc = integDataCheck->crc;
    integDataCheck->crc = 0;
    crc_cal = crc16_le(UINT16_MAX, (uint8_t const *)integDataCheck, sizeof(integStruct));
    // ESP_LOGW(TAG, "size %d", (uint8_t)sizeof(integStruct));
    // ESP_LOGW(TAG, "CRC %d and crccal %d and len %d and secret %s", crc, crc_cal, len, integDataCheck->sensorSecret);
    if (crc != crc_cal || strcmp(integDataCheck->sensorSecret, sensorSecret) != 0)
    {
        ESP_LOGI(TAG, "CRC/SECRET INVALID");
        return;
    }

    EspNowStruct *dataStruct = (EspNowStruct *)pvPortMalloc(sizeof(EspNowStruct));
    dataStruct->data = (uint8_t *)pvPortMalloc(len);
    memset(dataStruct->data, 0, len);
    memcpy(dataStruct->data, data, len);
    dataStruct->mode = integDataCheck->mode;
    dataStruct->devType = integDataCheck->devType;
    dataStruct->volt = integDataCheck->volt;
    dataStruct->cmnd = integDataCheck->cmnd;
    dataStruct->val = integDataCheck->val;
    strcpy(dataStruct->str, integDataCheck->str);
    // NEW register device!
    dataStruct->registerDev = integDataCheck->registerDev;
    dataStruct->volt = integDataCheck->volt;
    strcpy(dataStruct->devID, integDataCheck->devID);
    strcpy(dataStruct->meshID, integDataCheck->meshID);

    if (!Utilities_MacToStringShortVersion(mac_addr, dataStruct->shortDevMac, sizeof(dataStruct->shortDevMac)))
    {
        vPortFree(dataStruct->data);
        vPortFree(dataStruct);
        return;
    }
    memcpy(dataStruct->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    if (xQueueSendToBack(espNowQueue, &dataStruct, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(dataStruct->data);
        vPortFree(dataStruct);
        // Report back to AWS here letting us know that the queue is full and to wait for the next command
        // The queue most likely will neveer fill up but you never know
    }
}

void EspNow_SendCb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "Sent ESP Now data");
    }
    else
    {
        ESP_LOGI(TAG, "Could not send ESP Now data");
    }
}

#endif