#ifdef IPNODE
#include "includes/SpacrIO_commands.h"

static const char *TAG = "SIO_Cmnd";

#define totalPins 16
#define totalADC 4
uint8_t pinsEnabled[totalPins] = {0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3};
uint8_t muxCombi[totalADC][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
char *pinLabels[totalPins] = {"T1", "F2", "T8", "-", "T2", "T9", "T5", "D1", "T3", "T10", "T7", "-", "T4", "F1", "T6", "-"};
uint8_t adcPins[totalADC] = {adc1, adc3, adc2, adc4};
esp_adc_cal_characteristics_t *adcCharsIO = NULL;
//char macAddressStrIO[18];
//minimum sample period is 2.5seconds
uint32_t samplePeriod = 300000; //default set to 5 minutes
TaskHandle_t submissionTaskHandle = NULL;

void _submissionTask()
{
    adcCharsIO = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, VREF_IO, adcCharsIO);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(TAG, "Characterized using eFuse Vref");
    }
    else
    {
        ESP_LOGI(TAG, "Characterized using default vref");
    }
    while (true)
    {
        uint8_t pinCounter = 0;
        cJSON *readData = cJSON_CreateObject();
        for (uint8_t i = 0; i < 4; i++)
        {
            //Mux 1
            gpio_set_level(D1, muxCombi[i][0]);
            gpio_set_level(D2, muxCombi[i][1]);
            //mux 2
            gpio_set_level(D4, muxCombi[i][0]);
            gpio_set_level(D3, muxCombi[i][1]);

            vTaskDelay(500 / portTICK_RATE_MS);
            float fReadValue = 0;
            for (uint8_t y = 0; y < 4; y++)
            {
                if (pinsEnabled[pinCounter] == 1)
                {
                    //Digital reads
                    if (pinCounter == 3 || pinCounter == 7)
                    {
                        uint32_t ureadValue = gpio_get_level(adcPins[y]);
                        fReadValue = (float)ureadValue;
                    }
                    else if (pinCounter == 1 || pinCounter == 13)
                    {
                        //Flow meters
                        uint32_t ureadValue = NodeUtilities_ReadMilliVolts(adcPins[y], 100, adcCharsIO);
                        fReadValue = ((ureadValue / 1000.0) * (200000 + 100000)) / 100000;
                    }
                    else
                    {
                        //Thermistors
                        uint32_t ureadValue = NodeUtilities_ReadMilliVolts(adcPins[y], 100, adcCharsIO);
                        float adc_volts = ureadValue / 1000.0;
                        fReadValue = (10000.0 * adc_volts) / (3.3 - adc_volts);
                    }
                    cJSON_AddNumberToObject(readData, pinLabels[pinCounter], (truncf(fReadValue * 10.0) / 10.0));
                }
                pinCounter++;
            }
        }
        cJSON_AddItemToObject(readData, "devID", cJSON_CreateString(deviceMACStr));
        cJSON_AddNumberToObject(readData, "devType", devType);
        char *dataToSend = cJSON_PrintUnformatted(readData);
        //NodeUtilities_PrepareJsonAndSendToRoot(58, true, 0, true, dataToSend);
        cJSON_Delete(readData);
        free(dataToSend);
        if (samplePeriod < 2500)
            samplePeriod = 2500;
        vTaskDelay((samplePeriod - 2000) / portTICK_RATE_MS);
    }
}

void _initSIO()
{
    //Utilities_GetDeviceMac(macAddressStrIO);
    //Initialising ADC

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_channel_t adcChannel;
    adcChannel = NodeUtilities_AdcChannelLookup(adc1);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);
    adcChannel = NodeUtilities_AdcChannelLookup(adc2);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);
    adcChannel = NodeUtilities_AdcChannelLookup(adc3);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);

    //Initialising GPIOS
    NodeUtilities_InitGpios(outputMasksIO, GPIO_MODE_OUTPUT);
    NodeUtilities_InitGpios(inputMasksIO, GPIO_MODE_INPUT);
}

void _startADCSubmission()
{
    xTaskCreate(_submissionTask, "_submissionTask", 4096, NULL, 4, &submissionTaskHandle);
}

void _loadVoltageAndCutoff()
{
    uint16_t potVal = 256;
    uint8_t gndLevel = 0;
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    if (nvs_get_u16(nvsHandle, "potval_IO", &potVal) != ESP_OK)
    {
        nvs_close(nvsHandle);
    }
    if (nvs_get_u8(nvsHandle, "gndCutOff_IO", &gndLevel) != ESP_OK)
    {
        nvs_close(nvsHandle);
    }
    nvs_close(nvsHandle);
    setWiper(potVal);
    gpio_set_level(gndCutOff, gndLevel);
}

void _loadADCAndPeriod()
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    size_t pinsEnabledSize = sizeof(pinsEnabled);
    if (nvs_get_blob(nvsHandle, "adcsio", &pinsEnabled, &pinsEnabledSize) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    if (nvs_get_u32(nvsHandle, "period", &samplePeriod) != ESP_OK)
    {
        nvs_close(nvsHandle);
        return;
    }
    nvs_close(nvsHandle);
    // for (uint8_t i = 0; i < 16; i++){
    //         ESP_LOGI(TAG, "EnalbedOrDis %d", pinsEnabled[i]);
    // }
}

void _loadRelayPosition()
{
    uint8_t trigger = 0;
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    if (nvs_get_u8(nvsHandle, "relay_IO", &trigger) != ESP_OK)
    {
        nvs_close(nvsHandle);
    }
    nvs_close(nvsHandle);
    gpio_set_level(RELAY1, trigger);
}

void _loadEnableDisableSub()
{
    uint8_t enableDisable = 0;
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    if (nvs_get_u8(nvsHandle, "enDisSub", &enableDisable) != ESP_OK)
    {
        nvs_close(nvsHandle);
    }
    nvs_close(nvsHandle);
    if (enableDisable == 1)
        _startADCSubmission();
}

void SIO_Init()
{
    //Initialise spacrIO
    _initSIO();
    _loadADCAndPeriod();
    _loadEnableDisableSub();
    _loadVoltageAndCutoff();
    _loadRelayPosition();
}

//##############  BELOW ARE THE COMMANDS   #####################################
//##########################################################################

bool SIO_SetVoltage(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SIO_SetVoltage Function Called");
    if (structNodeReceived->dValue <= 1)
        gpio_set_level(gndCutOff, 0);
    else
        gpio_set_level(gndCutOff, 1);

    uint16_t potVal = ceil((((((16900 * 0.8) / (structNodeReceived->dValue - 0.8)) - 75.0) * 256.0) / 10000.0));
    if (potVal >= 256 || (structNodeReceived->dValue > 1 && structNodeReceived->dValue <= 2))
    {
        potVal = 256;
        setWiper(potVal);
        //vTaskDelete(NULL);
    }
    else if (potVal <= 35 || structNodeReceived->dValue >= 10)
    {
        potVal = 35;
        setWiper(potVal);
        //vTaskDelete(NULL);
    }
    setWiper(potVal);
    //Saving to NVS
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_set_u8(nvsHandle, "gndCutOff_IO", gpio_get_level(gndCutOff)) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_set_u16(nvsHandle, "potval_IO", potVal) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_commit(nvsHandle) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    nvs_close(nvsHandle);
    return true;
}

bool SIO_SelectADCs(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SIO_SelectADCs Function Called");
    if (structNodeReceived->arrValueSize >= totalPins)
    {
        for (uint8_t i = 0; i < structNodeReceived->arrValueSize; i++)
        {
            if (i == 11 || i == 15)
                continue; //REMOVE once the MUX has all the pins being USED
            if (structNodeReceived->arrValues[i] < 2)
                pinsEnabled[i] = structNodeReceived->arrValues[i];
        }
        nvs_handle nvsHandle;
        if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
            return false;
        size_t pinsEnabledSize = sizeof(pinsEnabled);
        if (nvs_set_blob(nvsHandle, "adcsio", &pinsEnabled, pinsEnabledSize) != ESP_OK)
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

        // for (uint8_t i = 0; i < structNodeReceived->arrValueSize; i++){
        //     ESP_LOGI(TAG, "EnalbedOrDis %d", structNodeReceived->arrValues[i]);
        // }
    }
    return false;
}

bool SIO_SetADCPeriod(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SIO_SetADCPeriod Function Called");
    //Converting seconds to milliseconds here
    samplePeriod = 1000 * (uint32_t)structNodeReceived->dValue;
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u32(nvsHandle, "period", samplePeriod) != ESP_OK)
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

bool SIO_EnableDisableSub(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SIO_EnableDisableSub Function Called");
    uint8_t enableDisable;
    if (structNodeReceived->dValue == 1)
    {
        enableDisable = 1;
        //enable submission
        if (submissionTaskHandle != NULL)
            return false;
        _startADCSubmission();
    }
    else
    {
        if (submissionTaskHandle != NULL)
        {
            if (adcCharsIO != NULL)
                free(adcCharsIO);
            vTaskDelete(submissionTaskHandle);
            submissionTaskHandle = NULL;
            adcCharsIO = NULL;
        }
        enableDisable = 0;
    }
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u8(nvsHandle, "enDisSub", enableDisable) != ESP_OK)
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

bool SIO_SetRelay(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SIO_SetRelay Function Called");
    uint8_t trigger = (uint8_t)structNodeReceived->dValue;
    trigger > 2 ? trigger = 1 : trigger;
    gpio_set_level(RELAY1, trigger);
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u8(nvsHandle, "relay_IO", trigger) != ESP_OK)
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
#endif