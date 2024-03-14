#ifdef IPNODE
#include "includes/node_commands.h"
#include "includes/node_utilities.h"
#include "includes/esp_now_utilities.h"
#include "includes/sensor_commands.h"

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
    gpio_set_level(NODE_OUTPUT_PIN_DEFAULT, value);
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
    // assuming groupID is a string = 1....20..30
    // group ID cannot be 0 as atoi returns 0 if it fails
    char groupMacAddr[18] = "01:00:5e:ae:ae:";
    if (strlen(structNodeReceived->cptrString) != 2)
        return false;
    strcat(groupMacAddr, structNodeReceived->cptrString);
    uint8_t groupMac[6] = {};
    if (!Utilities_StringToMac(groupMacAddr, groupMac))
        return false;

    // Getting group list
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
    {
        return false;
    }
    if (!NodeUtilities_LoadAndDeleteGroup(nvsHandle))
    {
        nvs_close(nvsHandle);
        return false;
    }
    if (nvs_set_blob(nvsHandle, "group", &groupMac, sizeof(groupMac)) != ESP_OK)
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
    if (esp_mesh_set_group_id((mesh_addr_t *)groupMac, 1) != ESP_OK)
        return false;
    return true;
}

bool DeleteGroup(NodeStruct_t *structNodeReceived)
{

    // init nvs handle
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;

    if (!NodeUtilities_LoadAndDeleteGroup(nvsHandle))
    {
        nvs_close(nvsHandle);
        return false;
    }
    nvs_close(nvsHandle);
    return true;
}

bool AssignSensorToSelf(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "AssignSensorToSelf Function Called");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    uint8_t sensorAction = DEFAULT_SENSOR_ACTION; // defaulting to just sensor data
    // validate sensor macaddress
    // the Sensor macaddress here must be without the ":"
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
    // the Sensor macaddress here must be without the ":"
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
    ESP_LOGI(TAG, "RemoveSensor Function Called");
    // mac of sensor needs to be without colons
    ESP_LOGI(TAG, "RemoveSensor");
    char sensorMac[13];
    if (!Utilities_ValidateHexString(structNodeReceived->cptrString, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    strcpy(sensorMac, structNodeReceived->cptrString);

    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;

    // remove sensorAction
    esp_err_t eraseErr = nvs_erase_key(nvsHandle, structNodeReceived->cptrString);
    if (eraseErr != ESP_OK)
    {
        if (eraseErr == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGI(TAG, "Sensor not assigned to node");
        }
        // nvs_close(nvsHandle);
        // return false;
        // Commented these out because we still would like to be able to send a mesasge to a sensor if it was autopaired (where sensor is not stored in NVS)
    }
    nvs_close(nvsHandle);

    // build sensorDataStruct
    sensorDataStruct dataToSendSensor;
    dataToSendSensor.cmnd = 301;
    dataToSendSensor.val = 0;
    strcpy(dataToSendSensor.str, "\0");
    strcpy(dataToSendSensor.sensorSecret, sensorSecret);
    dataToSendSensor.crc = 0;
    dataToSendSensor.crc = crc16_le(UINT16_MAX, (uint8_t const *)&dataToSendSensor, sizeof(sensorDataStruct));

    if (!Sensor_SendCommandToSensor(sensorMac, &dataToSendSensor))
    {
        return false;
    }

    ESP_LOGI(TAG, "RemoveSensor Done");
    return true;
}

bool AssignSensorToNodeOrGroup(NodeStruct_t *structNodeReceived)
{
    // macofSensor_mac:of:Node/mac:of:group
    ESP_LOGI(TAG, "AssignSensorToNodeOrGroup");
    char *token;
    char macOfSensor[13];
    // Getting the mac address in string of the sensor
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
    // Convert string mac short version to uint8_t mac address
    // Converting string to mac does not work with the missing ':'
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
    // mac of sensor needs to be without colons
    ESP_LOGI(TAG, "ChangePortPIR");
    // validate hex string
    if (!Utilities_ValidateHexString(structNodeReceived->cptrString, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    char sensorMac[13];
    strcpy(sensorMac, structNodeReceived->cptrString);
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
    nvs_close(nvsHandle);

    // build sensorDataStruct
    sensorDataStruct dataToSendSensor;
    dataToSendSensor.cmnd = 302;
    dataToSendSensor.val = structNodeReceived->dValue;
    strcpy(dataToSendSensor.str, "\0");
    strcpy(dataToSendSensor.sensorSecret, sensorSecret);
    dataToSendSensor.crc = 0;
    dataToSendSensor.crc = crc16_le(UINT16_MAX, (uint8_t const *)&dataToSendSensor, sizeof(sensorDataStruct));

    if (!Sensor_SendCommandToSensor(sensorMac, &dataToSendSensor))
    {
        return false;
    }
    return true;

    ESP_LOGI(TAG, "jChangePort Done");
    return true;
}

bool OverrideNodeType(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "OverrideNodeType Function Called");
    // 1 - power node, 2 - spacr IO, 3 - contactor
    uint8_t nodeType = (uint8_t)structNodeReceived->dValue;
    if (nodeType > 3 || nodeType <= 0)
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
    // 1 - power node, 2 - spacr IO, 3 - contactor
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
        // Send this error to NVS
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

bool SetAutoSensorPairing(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SetAutoSensorPairing Function Called");
    uint8_t value = (uint8_t)structNodeReceived->dValue;
    if (value >= 1)
    {

        autoSensorPairing = 1;
        ESP_LOGI(TAG, "Auto sensor pairing turned on");
    }
    else
    {
        autoSensorPairing = 0;
        ESP_LOGI(TAG, "Auto sensor pairing turned off");
    }
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u8(nvsHandle, "autoSensorPair", autoSensorPairing) != ESP_OK)
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

bool SetSensorMeshID(NodeStruct_t *structNodeReceived)
{
    // str: "macOfSensor_meshID"
    ESP_LOGI(TAG, "SetSensorMeshID Function Called");
    char *copy;
    char *token;
    char sensorMac[13];
    char meshID[MESH_ID_LENGTH];

    // get copy of structNodeReceived->cptrString for validation
    size_t len = strlen(structNodeReceived->cptrString);
    copy = malloc(len + 1);
    if (copy == NULL)
    {
        return false;
    }
    strcpy(copy, structNodeReceived->cptrString);

    // get and validate sensor mac address
    token = strtok(copy, "_");
    if (token == NULL)
    {
        ESP_LOGI(TAG, "Sensor Mac Address not found / input string format incorrect");
        free(copy);
        return false;
    }
    strcpy(sensorMac, token);
    if (!Utilities_ValidateHexString(sensorMac, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        free(copy);
        return false;
    }

    // get and validate meshID
    token = strtok(NULL, "_");
    if (token == NULL || strlen(token) >= MESH_ID_LENGTH)
    {
        ESP_LOGI(TAG, "Invalid meshID");
        free(copy);
        return false;
    }
    strcpy(meshID, token);

    // build sensorDataStruct
    sensorDataStruct dataToSendSensor;
    dataToSendSensor.cmnd = 303;
    dataToSendSensor.val = 0;
    memset(dataToSendSensor.str, '\0', sizeof(dataToSendSensor.str));
    strcpy(dataToSendSensor.str, meshID);
    strcpy(dataToSendSensor.sensorSecret, sensorSecret);
    dataToSendSensor.crc = 0;
    dataToSendSensor.crc = crc16_le(UINT16_MAX, (uint8_t const *)&dataToSendSensor, sizeof(sensorDataStruct));

    if (!Sensor_SendCommandToSensor(sensorMac, &dataToSendSensor))
    {
        free(copy);
        return false;
    }
    free(copy);
    ESP_LOGI(TAG, "SetSensorMeshID done");
    return true;
}

bool SendSensorFirmware(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SendSensorFirmware Function Called");
    // get sha256 of partition

    const esp_partition_t *data_partition = esp_ota_get_next_update_partition(NULL);
    esp_partition_get_sha256(data_partition, sha_256);

    // assume that we get a list of destination addresses as input, and number of sensors in arrValueSize
    size_t numOfSensors = structNodeReceived->arrValueSize / ESPNOW_ADDR_LEN;
    double *sensor_address_list = structNodeReceived->arrValues;
    if (numOfSensors == 0)
        return false;
    char sensorMac[13];
    espnow_addr_t *dest_addr_list = ESP_MALLOC(numOfSensors * ESPNOW_ADDR_LEN);
    for (size_t i = 0; i < numOfSensors; i++)
    {
        for (int j = 0; j < ESPNOW_ADDR_LEN; j++)
        {
            dest_addr_list[i][j] = (uint8_t)sensor_address_list[i * ESPNOW_ADDR_LEN + j];
        }
        Utilities_MacToStringShortVersion((uint8_t *)dest_addr_list[i], sensorMac, sizeof(sensorMac));

        // build sensorDataStruct
        sensorDataStruct dataToSendSensor;
        dataToSendSensor.cmnd = 304;
        dataToSendSensor.val = 0;
        memset(dataToSendSensor.str, '\0', sizeof(dataToSendSensor.str));
        strcpy(dataToSendSensor.sensorSecret, sensorSecret);
        dataToSendSensor.crc = 0;
        dataToSendSensor.crc = crc16_le(UINT16_MAX, (uint8_t const *)&dataToSendSensor, sizeof(sensorDataStruct));

        if (!Sensor_SendCommandToSensor(sensorMac, &dataToSendSensor))
        {
            ESP_FREE(dest_addr_list);
            return false;
        }
    }
    ESP_FREE(dest_addr_list);
    // mupgrade_clear is defined in spacr-espmdf
    mupgrade_clear();
    // get firmware size
    firmware_size = upgradeFileSize;
    if (firmware_size == 0)
    {
        ESP_LOGI(TAG, "SENSOR FIRMWARE NOT DOWNLOADED");
        return false;
    }
    return true;
}

bool ClearSensorCommand(NodeStruct_t *structNodeReceived)
{
    // Checking if this mac address is for this node or a group or another node
    char macStringMsg[14];

    if (!Utilities_ValidateHexString(structNodeReceived->cptrString, 12))
    {
        ESP_LOGI(TAG, "String is not valid hexadecimal string of length 12");
        return false;
    }
    strcpy(macStringMsg, structNodeReceived->cptrString);
    strcat(macStringMsg, "m");
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_erase_key(nvsHandle, macStringMsg) != ESP_OK)
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

bool ClearNVS(NodeStruct_t *structNodeReceived)
{
    // This will clear ALL values in the NVS
    ESP_LOGI(TAG, "ClearNVS Function Called");
    nvs_flash_deinit();
    nvs_flash_erase();
    // the return below the restart will not run
    esp_restart();
    return true;
}

bool SendFirmwareVersion(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SendFirmwareVersion Function Called");
    cJSON *cJFirmwareVersion = cJSON_CreateObject();
    if (!cJFirmwareVersion)
    {
        ESP_LOGI(TAG, "Error creating cJFirmwareVersion");
        // Send this error to NVS
        return false;
    }
    else
    {
        cJSON_AddStringToObject(cJFirmwareVersion, "Git_branch", GIT_BRANCH);
        cJSON_AddStringToObject(cJFirmwareVersion, "Commit_hash", GIT_HASH);
        cJSON_AddStringToObject(cJFirmwareVersion, "Commit_time", GIT_TIME);
        char *dataToSend = cJSON_PrintUnformatted(cJFirmwareVersion);
        NodeUtilities_PrepareJsonAndSendToRoot(60, 0, dataToSend);
        free(dataToSend);
        cJSON_Delete(cJFirmwareVersion);
        return true;
    }
}
#endif