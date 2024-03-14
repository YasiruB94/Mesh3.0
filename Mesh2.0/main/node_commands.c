#ifdef IPNODE
#include "includes/node_commands.h"
#include "includes/node_utilities.h"

static const char *TAG = "NodeCommands";

bool RestartNode(NodeStruct_t *structNodeReceived)
{
    ESP_LOGW(TAG, "Node will restart in 3 seconds...");
    vTaskDelay(3000 / portTICK_RATE_MS);
    esp_restart();
    return true;
}

bool TestNode(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "Test Node Function Called");
    cJSON *nodeData = cJSON_CreateObject();
    cJSON_AddItemToObject(nodeData, "devID", cJSON_CreateString(deviceMACStr));
    cJSON_AddNumberToObject(nodeData, "status", 1);
    char *dataToSend = cJSON_PrintUnformatted(nodeData);
    NodeUtilities_PrepareJsonAndSendToRoot(60, 0, dataToSend);
    cJSON_Delete(nodeData);
    free(dataToSend);
    return true;
}

bool SetNodeOutput(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SetNodeOutput Function Called");
    uint8_t value = (uint8_t)structNodeReceived->dValue;
    if (value >= 1)
    {
        value = 1;
    }
    else
    {
        value = 0;
    }
    gpio_set_level(nodeOutputPin, value);
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u8(nvsHandle, "nodeOutput", value) != ESP_OK)
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
    return true;
}

bool CreateGroup(NodeStruct_t *structNodeReceived)
{
    //assuming groupID is a string = 1....20..30
    //group ID cannot be 0 as atoi returns 0 if it fails
    char groupMacAddr[16] = "01:00:5e:ae:ae:";
    uint8_t groupIDNum = atoi(structNodeReceived->cptrString);
    if (groupIDNum == 0)
        return false;

    //Getting group list
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;

    //getting the size of the mac list from the blob
    size_t groupListSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, "groups", NULL, &groupListSize);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(nvsHandle);
        return false;
    }
    //added a one to the size because we are adding another ID to the list
    uint8_t groupIDS[groupListSize + 1];
    //defining a new string mac
    char strMac[18];
    uint8_t numMac[6];
    //no groups found so adding a new one
    if (groupListSize == 0)
        goto addToListAndSave;

    //getting the group mac list
    if (nvs_get_blob(nvsHandle, "groups", &groupIDS, &groupListSize) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }

    //Chcecking to see if it already exists in the list (maybe adding to a group didnt work but it saved to the list already)
    for (uint8_t i = 0; i < groupListSize; i++)
    {
        if (groupIDS[i] == groupIDNum)
        {
            goto setingAsGroup;
        }
    }

addToListAndSave:
    groupIDS[groupListSize] = groupIDNum;
    if (nvs_set_blob(nvsHandle, "groups", &groupIDS, sizeof(groupIDS)) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    if (nvs_commit(nvsHandle) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }

setingAsGroup:
    //copying default mac to new full mac size
    strcpy(strMac, groupMacAddr);
    strcat(strMac, structNodeReceived->cptrString);
    if (!Utilities_StringToMac(strMac, numMac))
    {
        nvs_close(nvsHandle);
        return false;
    }
    if (esp_mesh_set_group_id((mesh_addr_t *)numMac, 1) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
    return true;
}

bool DeleteGroup(NodeStruct_t *structNodeReceived)
{
    //assuming groupID is a string = 1....20..30
    //group ID cannot be 0 as atoi returns 0 if it fails
    uint8_t groupIDNum = atoi(structNodeReceived->cptrString);
    if (groupIDNum == 0)
        return false;

    char groupMacAdd[16] = "01:00:5e:ae:ae:";

    //init nvs handle
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;

    //getting the size of the mac list from the blob
    size_t groupListSize = 0;
    esp_err_t err = nvs_get_blob(nvsHandle, "groups", NULL, &groupListSize);
    if (err != ESP_OK || groupListSize == 0)
    {
        nvs_close(nvsHandle);
        return false;
    }
    //getting the group mac list
    uint8_t groupIDS[groupListSize];
    if (nvs_get_blob(nvsHandle, "groups", &groupIDS, &groupListSize) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return false;
    }
    ESP_LOGI(TAG, "Group list size %d", groupListSize);
    char strMac[18];
    uint8_t numMac[6];
    ///check if only 1 number left, and thats the one to be removed, we can erase the key
    if (groupListSize == 1 && groupIDS[0] == groupIDNum)
    {
        if (nvs_erase_key(nvsHandle, "groups") != ESP_OK)
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
        goto removingGroup;
        //return true;
    }
    else if (groupListSize == 1)
    {
        ESP_LOGI(TAG, "Size 1");
        nvs_close(nvsHandle);
        return false;
    }

    uint8_t numToRemove = groupIDNum;
    for (uint8_t i = 0; i < groupListSize; i++)
    {
        if (groupIDS[i] == numToRemove && i != groupListSize - 1)
        {
            groupIDS[i] = groupIDS[i + 1];
            numToRemove = groupIDS[i];
        }
    }
    if (nvs_set_blob(nvsHandle, "groups", &groupIDS, groupListSize - 1) != ESP_OK)
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

removingGroup:
    //removing from mesh ID
    strcpy(strMac, groupMacAdd);
    strcat(strMac, structNodeReceived->cptrString);
    if (!Utilities_StringToMac(strMac, numMac))
        return false;
    if (esp_mesh_delete_group_id((mesh_addr_t *)numMac, 1) != ESP_OK)
        return false;
    ESP_LOGI(TAG, "Deleted");
    return true;
}

bool AssignSensorToSelf(NodeStruct_t *structNodeReceived)
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    uint8_t sensorAction = 0; //defaulting to just sensor data
    //validate sensor macaddress
    //the Sensor macaddress here must be without the ":"
    if (!(Utilities_ValidateHexString(structNodeReceived->cptrString, 12)))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        nvs_close(nvsHandle);
        return false;
    }
    if (nvs_set_blob(nvsHandle, structNodeReceived->cptrString, &sensorAction, sizeof(sensorAction)) != ESP_OK)
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
    ESP_LOGI(TAG, "Done");
    return true;
}

bool EnableOrDisableSensorAction(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "Enable or disable sensor action");
    if (!Utilities_ValidateHexString(structNodeReceived->cptrString, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    size_t sensorActionSize = 0;
    nvs_handle nvsHandle;

    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_get_blob(nvsHandle, structNodeReceived->cptrString, NULL, &sensorActionSize) != ESP_OK)
    {
        ESP_LOGI(TAG, "Sensor not assigned to node");
        nvs_close(nvsHandle);
        return false;
    }
    uint8_t sensorAction = (uint8_t)structNodeReceived->dValue;
    //the Sensor macaddress here must be without the ":"
    if (nvs_set_blob(nvsHandle, structNodeReceived->cptrString, &sensorAction, sizeof(sensorAction)) != ESP_OK)
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
    ESP_LOGI(TAG, "Done Disabling");
    return true;
}

bool RemoveSensor(NodeStruct_t *structNodeReceived)
{
    //mac of sensor needs to be without colons
    ESP_LOGI(TAG, "RemoveSensor");
    char sensorMacM[14];
    strcpy(sensorMacM, structNodeReceived->cptrString);
    strcat(sensorMacM, "m");
    if (!Utilities_ValidateHexString(structNodeReceived->cptrString, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    esp_err_t eraseErr = nvs_erase_key(nvsHandle, structNodeReceived->cptrString);
    if (eraseErr != ESP_OK)
    {
        if (eraseErr == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGI(TAG, "Sensor not assigned to node");
        }
        nvs_close(nvsHandle);
        return false;
    }
    //the Sensor macaddress here must be without the ":"
    cJSON *jRemoveSensor = cJSON_CreateObject();
    cJSON_AddNumberToObject(jRemoveSensor, "cmnd", 301);
    //char devMacStr[18];
    //Utilities_GetDeviceMac(devMacStr);
    cJSON_AddItemToObject(jRemoveSensor, "val", cJSON_CreateString(deviceMACStr));
    char *dataToSave = cJSON_PrintUnformatted(jRemoveSensor);
    if (nvs_set_str(nvsHandle, sensorMacM, dataToSave) != ESP_OK)
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
    cJSON_Delete(jRemoveSensor);
    free(dataToSave);
    ESP_LOGI(TAG, "RemoveSensor Done");
    return true;
}

bool AssignSensorToNodeOrGroup(NodeStruct_t *structNodeReceived)
{
    //macofSensor_mac:of:Node/mac:of:group
    ESP_LOGI(TAG, "AssignSensorToNodeOrGroup");
    char *token;
    char macOfSensor[13];
    //Getting the mac address in string of the sensor
    token = strtok(structNodeReceived->cptrString, "_");
    strcpy(macOfSensor, token);
    token = strtok(NULL, "_");
    uint8_t nodeMac[6] = {0};
    // validate string mac
    if (!Utilities_ValidateHexString(macOfSensor, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    // validate node/group mac
    if (!(Utilities_ValidateMacAddress(token)))
    {
        ESP_LOGI(TAG, "Node/Group MAC is not valid MAC address");
        return false;
    }
    //Convert string mac short version to uint8_t mac address
    //Converting string to mac does not work with the missing ':'
    Utilities_StringToMac(token, nodeMac);

    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    size_t sensorActionSize = 0;
    if (nvs_get_blob(nvsHandle, structNodeReceived->cptrString, NULL, &sensorActionSize) != ESP_OK)
    {
        ESP_LOGI(TAG, "Sensor not assigned to node");
        nvs_close(nvsHandle);
        return false;
    }
    if (nvs_set_blob(nvsHandle, macOfSensor, &nodeMac, sizeof(nodeMac)) != ESP_OK)
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
    ESP_LOGI(TAG, "Done");
    return true;
}

bool ChangePortPIR(NodeStruct_t *structNodeReceived)
{
    //mac of sensor needs to be without colons
    ESP_LOGI(TAG, "ChangePortPIR");
    // validate hex string
    if (!Utilities_ValidateHexString(structNodeReceived->cptrString, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    char sensorMacM[14];
    strcpy(sensorMacM, structNodeReceived->cptrString);
    strcat(sensorMacM, "m");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    // validate value range
    if (structNodeReceived->dValue < 0 || structNodeReceived->dValue > UINT16_MAX)
    {
        ESP_LOGI(TAG, "Value out of range for uint16_t");
        nvs_close(nvsHandle);
        return false;
    }
    // validate sensor existence
    size_t sensorActionSize = 0;
    if (nvs_get_blob(nvsHandle, structNodeReceived->cptrString, NULL, &sensorActionSize) != ESP_OK)
    {
        ESP_LOGI(TAG, "Sensor not assigned to node");
        nvs_close(nvsHandle);
        return false;
    }
    //the Sensor macaddress here must be without the ":"
    cJSON *jChangePort = cJSON_CreateObject();
    cJSON_AddNumberToObject(jChangePort, "cmnd", 302);
    //char devMacStr[18];
    //Utilities_GetDeviceMac(devMacStr);
    cJSON_AddNumberToObject(jChangePort, "val", structNodeReceived->dValue);
    char *dataToSave = cJSON_PrintUnformatted(jChangePort);
    if (nvs_set_str(nvsHandle, sensorMacM, dataToSave) != ESP_OK)
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
    cJSON_Delete(jChangePort);
    free(dataToSave);
    ESP_LOGI(TAG, "jChangePort Done");
    return true;
}

bool OverrideNodeType(NodeStruct_t *structNodeReceived)
{   //GW required changes: changes made in the function wherever needed
    ESP_LOGI(TAG, "OverrideNodeType Function Called");
    //1 - power node, 2 - spacr IO, 3 - contactor, 4 - Gateway
    uint8_t nodeType = (uint8_t)structNodeReceived->dValue;
    if (nodeType > 4 || nodeType <= 0)
        nodeType = 1;
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u8(nvsHandle, "nodeType", nodeType) != ESP_OK)
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
    ESP_LOGI(TAG, "OverrideNodeType initiated");
    esp_restart();
    // P.S. calling esp_restart() will stop further processing, the following line will not be returned
    return true;
}

bool RemoveNodeTypeOverride(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "OverrideNodeType Function Called");
    //1 - power node, 2 - spacr IO, 3 - contactor, 4 - Gateway
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_erase_key(nvsHandle, "nodeType") != ESP_OK)
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
    ESP_LOGI(TAG, "NodeType removed");
    // P.S. calling esp_restart() will stop further processing, the following line will not be returned
    esp_restart();
    return true;
}

bool GetDaisyChainedCurrent(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "GetDaisyChainedCurrent Function Called");
    ESP_LOGI(TAG, "Value Received: %f", structNodeReceived->dValue);
    ESP_LOGI(TAG, "String Received: %s", structNodeReceived->cptrString);
    return true;
}

bool GetFirmwareVersion(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "GetFirmwareVersion Function Called");
    ESP_LOGI(TAG, "Value Received: %f", structNodeReceived->dValue);
    ESP_LOGI(TAG, "String Received: %s", structNodeReceived->cptrString);
    return true;
}

bool GetNodeOutput(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "GetNodeOutput Function Called");
    ESP_LOGI(TAG, "Value Received: %f", structNodeReceived->dValue);
    ESP_LOGI(TAG, "String Received: %s", structNodeReceived->cptrString);
    return true;
}

bool GetParentRSSI(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "Getting RSSI of parent node");
    cJSON *cJRssi = cJSON_CreateObject();
    if (cJRssi == NULL)
    {
        ESP_LOGE(TAG, "Error Creating json object");
        return false;
    }
    cJSON_AddNumberToObject(cJRssi, "parentRssi", mwifi_get_parent_rssi());
    cJSON_AddItemToObject(cJRssi, "devID", cJSON_CreateString(deviceMACStr));
    char *dataToSend = cJSON_PrintUnformatted(cJRssi);
    NodeUtilities_PrepareJsonAndSendToRoot(60, 0, dataToSend);
    free(dataToSend);
    cJSON_Delete(cJRssi);
    return true;
}

bool GetNodeChildren(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "Getting Children of current node");

    wifi_sta_list_t wifi_sta_list = {0x0};
    esp_wifi_ap_get_sta_list(&wifi_sta_list);

    cJSON *cJChildMacs = cJSON_CreateObject();

    if (!cJChildMacs)
    {
        ESP_LOGI(TAG, "Error creating cJChildMacs");
        //Send this error to NVS
        return false;
    }
    else
    {
        cJSON *arrayNode = cJSON_CreateArray();
        if (!arrayNode)
            return false;
        for (int i = 0; i < wifi_sta_list.num; i++)
        {
            cJSON_AddItemToArray(arrayNode, cJSON_CreateString((char *)(wifi_sta_list.sta[i].mac)));
        }
        cJSON_AddItemToObject(cJChildMacs, "childNodes", arrayNode);
        char *dataToSend = cJSON_PrintUnformatted(cJChildMacs);
        NodeUtilities_PrepareJsonAndSendToRoot(60, 0, dataToSend);
        free(dataToSend);
        cJSON_Delete(cJChildMacs);
        return true;
    }
}

#endif