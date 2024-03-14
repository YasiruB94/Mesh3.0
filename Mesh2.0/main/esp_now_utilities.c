#ifdef IPNODE
#include "includes/esp_now_utilities.h"
static const char *TAG = "EspNowUtilities";

QueueHandle_t espNowQueue;

void EspNow_Init()
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(EspNow_RecvCb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(EspNow_SendCb));
}

void EspNow_CreateEspNowQueue()
{
    //created a queue with maxumum of 15 commands, and a item size of a char pointer
    espNowQueue = xQueueCreate(15, sizeof(EspNowStruct *));
    if (espNowQueue == NULL)
    {
        ESP_LOGE(TAG, "Could not create ESP NOW queue");
        //need to restart and let the backend know
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

void EspNow_ParseData(EspNowStruct *recv_cb, size_t *required_size, uint8_t *sensorAction, char *macOfNodeStr)
{
    //Checking if this mac address is for this node or a group or another node
    ESP_LOGW(TAG, "Parsing ESP NOW data");
    // char macString[13];
    // if (!Utilities_MacToStringShortVersion(recv_cb->mac_addr, macString, sizeof(macString))) return;
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    //size_t required_size;
    //if (nvs_get_blob(nvsHandle, recv_cb->macStr, NULL, &required_size) != ESP_OK) return;

    //Value is sensor action and is intended for this node.
    if (*required_size == 1)
    {
        ESP_LOGW(TAG, "Sensor data is action");
        ESP_LOGW(TAG, "size %d", (int)*required_size);
        if (nvs_get_blob(nvsHandle, recv_cb->macStr, sensorAction, required_size) != ESP_OK)
        {
            nvs_close(nvsHandle);
        }
        ESP_LOGW(TAG, "Here sensor action %d", *sensorAction);
        if (*sensorAction == 1)
        {
            strcpy(macOfNodeStr, deviceMACStr);
            ESP_LOGW(TAG, "Sensor data is for me");
            char *cptrData = pvPortMalloc(recv_cb->data_len);
            memset(cptrData, 0, recv_cb->data_len);
            memcpy(cptrData, recv_cb->data, recv_cb->data_len);
            if (xQueueSendToBack(nodeReadQueue, &cptrData, (TickType_t)0) != pdPASS)
            {
                ESP_LOGE(TAG, "Queue is full");
                vPortFree(cptrData);
                //Report back to Root here letting us know that the queue is full and to wait for the next command
                //The queue most likely will neveer fill up but you never know
            }
        }
        //Its not a sensor action its meant for a group or a node
    }
    else
    {
        *sensorAction = 1;
        ESP_LOGW(TAG, "Sensor data is for group or node");
        uint8_t macForNodeOrGroup[*required_size];
        if (nvs_get_blob(nvsHandle, recv_cb->macStr, &macForNodeOrGroup, required_size) != ESP_OK)
        {
            nvs_close(nvsHandle);
            return;
        }
        Utilities_MacToString(macForNodeOrGroup, macOfNodeStr, 18);
        if (NodeUtilities_compareWithGroupMac(macForNodeOrGroup))
        {
            NodeUtilities_SendDataToGroup(macForNodeOrGroup, (char *)recv_cb->data);
            //Implement group functionality here
        }
        else
        {
            ESP_LOGW(TAG, "Sensor data is sending to node");
            NodeUtilities_SendToNode(macForNodeOrGroup, (char *)recv_cb->data);
        }
    }
    nvs_close(nvsHandle);
}

void EspNow_RecvCb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    // example_espnow_event_t evt;
    cJSON *dataParse = cJSON_Parse((char *)data);
    uint8_t espCmp = strcmp((char *)data, "LFN");
    if (mac_addr == NULL || data == NULL || len < 3 || (dataParse == NULL && espCmp != 0)) // Added the parse and the strcmp to remove noise
    {
        //ESP_LOGW(TAG, "Received Bad data from ESP-NOW");
        cJSON_Delete(dataParse);
        return;
    }
    cJSON_Delete(dataParse);
    ESP_LOGI(TAG, "ESP Now received, addr: " MACSTR ", data: %s\n", MAC2STR(mac_addr), (char *)data);
    //checking if mac we're expecting data from this sensor
    char macString[13];
    if (!Utilities_MacToStringShortVersion(mac_addr, macString, sizeof(macString)))
        return;

    EspNowStruct *recv_cb = pvPortMalloc(sizeof(EspNowStruct));
    recv_cb->data = (uint8_t *)pvPortMalloc(len);
    recv_cb->macStr = (char *)pvPortMalloc(13);
    memset(recv_cb->data, 0, len);
    memset(recv_cb->macStr, 0, sizeof(macString));

    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    memcpy(recv_cb->data, data, len);
    memcpy(recv_cb->macStr, macString, sizeof(macString));
    recv_cb->data_len = len;
    //ESP_LOGW(TAG, "Passing to Queue");
    //ESP_LOGW(TAG, "Sensor MAc %s", recv_cb->macStr);
    //ESP_LOGI(TAG, "Leaf received, addr: " MACSTR ", data: %s\n", MAC2STR(mac_addr), (char *)data);
    if (xQueueSendToBack(espNowQueue, &recv_cb, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(recv_cb->data);
        vPortFree(recv_cb->macStr);
        vPortFree(recv_cb);
        //Report back to AWS here letting us know that the queue is full and to wait for the next command
        //The queue most likely will neveer fill up but you never know
    }
    //memset(recv_cb->data, 0, len);
    //memset(recv_cb->macStr, 0, sizeof(macString));
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