#ifdef IPNODE
#include "includes/SpacrContactor_commands.h"

static const char *TAG = "SC_Cmnd";

uint8_t relayPins[4] = {relay1, relay2, relay3, relay4};

void _setRelays(uint8_t *positions, uint8_t length)
{
    //ESP_LOGI(TAG, "Length of positions %d", length);
    if (length >= 4)
    {
        for (uint8_t i = 0; i < length; i++)
        {
            if (positions[i] < 2)
                gpio_set_level(relayPins[i], positions[i]);
        }
    }
}

void _loadRelayPositions()
{
    uint8_t positions[4] = {0, 0, 0, 0};
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    size_t positionSize = sizeof(positions);
    if (nvs_get_blob(nvsHandle, "relays", &positions, &positionSize) != ESP_OK)
    {
        nvs_close(nvsHandle);
    }
    nvs_close(nvsHandle);
    // for(uint8_t i = 0; i < 4; i++){
    //     ESP_LOGI(TAG, "Pin values %d", positions[i]);
    // }
    _setRelays(positions, sizeof(positions) / sizeof(positions[0]));
}

void _initGpios(uint64_t mask, gpio_mode_t mode)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = mode;
    io_conf.pin_bit_mask = mask;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void SC_Init()
{
    NodeUtilities_InitGpios(relayMasks, GPIO_MODE_OUTPUT);
    _loadRelayPositions();
}

//##############  BELOW ARE THE COMMANDS   #####################################
//##########################################################################

bool SC_SetRelays(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SC_SetRelays Function Called");
    _setRelays(structNodeReceived->arrValues, structNodeReceived->arrValueSize);
    // for(uint8_t i = 0; i < 4; i++){
    //     ESP_LOGI(TAG, "Pin values %d", structNodeReceived->arrValues[i]);
    // }
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_blob(nvsHandle, "relays", structNodeReceived->arrValues, (size_t)structNodeReceived->arrValueSize) != ESP_OK)
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
    //ESP_LOGI(TAG, "Saved");
}

bool SC_SelectADCs(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SC_SelectADCs Function Called");
    // if (structNodeReceived->arrValueSize >=totalPins){
    //     for (uint8_t i = 0; i < structNodeReceived->arrValueSize; i++){
    //         if (i == 11 || i == 15) continue; //REMOVE once the MUX has all the pins being USED
    //         if (structNodeReceived->arrValues[i] < 2) pinsEnabled[i] = structNodeReceived->arrValues[i];
    //     }
    //     nvs_handle nvsHandle;
    //     if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK) return;
    //     size_t pinsEnabledSize = sizeof(pinsEnabled);
    //     if (nvs_set_blob(nvsHandle, "adcsio", &pinsEnabled, pinsEnabledSize) != ESP_OK) return;
    //     if (nvs_commit(nvsHandle) != ESP_OK) return;
    //     nvs_close(nvsHandle);

    //     // for (uint8_t i = 0; i < structNodeReceived->arrValueSize; i++){
    //     //     ESP_LOGI(TAG, "EnalbedOrDis %d", structNodeReceived->arrValues[i]);
    //     // }
    // }
    return true;
}

bool SC_SetADCPeriod(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SC_SetADCPeriod Function Called");
    return true;
}

bool SC_EnableDisableDigiReads(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SC_EnableDisableDigiReads Function Called");
    return true;
}

#endif