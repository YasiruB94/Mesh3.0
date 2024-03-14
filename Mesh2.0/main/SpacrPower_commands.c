#ifdef IPNODE
#include "includes/SpacrPower_commands.h"

static const char *TAG = "SP_Cmnd";

static TaskHandle_t submissionTaskHandle = NULL;
uint8_t pwm0 = 0;
uint8_t pwm1 = 0;
uint8_t currentGPIO = 34;
esp_adc_cal_characteristics_t *currentCals = NULL;
#define VREF_power 1100

//time for current measurement
TimerHandle_t CVSTimer;

/* Prepare the LEDC PWM timer configuration */
ledc_timer_config_t ledc_timer1 = {
    .duty_resolution = LEDC_TIMER_10_BIT, // resolution of PWM duty
    .freq_hz = LEDC_FREQ,                 // frequency of PWM signal
    .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
    .timer_num = LEDC_TIMER_1,            // timer index
};
/* Prepare the LEDC PWM channel configuration */
ledc_channel_config_t ledc_channel1 = {
    .channel = LEDC_CHANNEL_1,
    .duty = MIN_DUTY_VALUE,
    .gpio_num = GPIO_NUM_18, // for Q1 - Port1 of LightHAT
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint = 0,
    .timer_sel = LEDC_TIMER_1,
    .intr_type = LEDC_INTR_DISABLE,
};
ledc_channel_config_t ledc_channel2 = {
    .channel = LEDC_CHANNEL_2,
    .duty = MIN_DUTY_VALUE,
    .gpio_num = GPIO_NUM_19, // For Q2 - Port2 of LightHAT
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint = 0,
    .timer_sel = LEDC_TIMER_1,
    .intr_type = LEDC_INTR_DISABLE,
};

/**
 * @brief This Function Initializes the PWM channels being used for the LightHAT
 */
void _initLedDriver()
{
    esp_err_t ret;
    ret = ledc_timer_config(&ledc_timer1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure LEDC timer");
        return false;
    }
    ret = ledc_channel_config(&ledc_channel1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure LEDC channel");
        return false;
    }
    ret = ledc_channel_config(&ledc_channel2);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure LEDC channel");
        return false;
    }
    ledc_fade_func_install(0);
    vTaskDelay(100 / portTICK_RATE_MS); // Time required to let the Fade func installed
    return true;
}

void _setPwm(uint8_t value, uint8_t port)
{
    value > 100 ? value = 100 : value;
    uint32_t dutyToSet = ceil(((double)value / 100.0) * MAX_DUTY_VALUE);
    if (port == 0)
    {
        pwm0 = (uint8_t)value;
        ledc_set_fade_time_and_start(ledc_channel1.speed_mode, ledc_channel1.channel, dutyToSet, LEDC_FADE_TIME_MS, LEDC_FADE_NO_WAIT);
    }
    else if (port == 1)
    {
        pwm1 = (uint8_t)value;
        ledc_set_fade_time_and_start(ledc_channel2.speed_mode, ledc_channel2.channel, dutyToSet, LEDC_FADE_TIME_MS, LEDC_FADE_NO_WAIT);
    }
    else
    {
        pwm0 = pwm1 = (uint8_t)value;
        ledc_set_fade_time_and_start(ledc_channel1.speed_mode, ledc_channel1.channel, dutyToSet, LEDC_FADE_TIME_MS, LEDC_FADE_NO_WAIT);
        ledc_set_fade_time_and_start(ledc_channel2.speed_mode, ledc_channel2.channel, dutyToSet, LEDC_FADE_TIME_MS, LEDC_FADE_NO_WAIT);
    }
}

void _loadVoltageAndPWMValues()
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        ESP_LOGE(TAG, "Log to AWSS");
    uint16_t potVal = MIN_POT_VALUE;
    if (nvs_get_u8(nvsHandle, "pwm0", &pwm0) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
    }
    if (nvs_get_u8(nvsHandle, "pwm1", &pwm1) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
    }
    if (nvs_get_u16(nvsHandle, "potval_SP", &potVal) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
    }
    nvs_close(nvsHandle);
    setWiper(potVal); //Setting volys
    _setPwm(pwm0, 0);
    _setPwm(pwm1, 1);
}

void _loadCurrentSubTask()
{
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return;
    uint8_t enableOrDisable = 0;
    if (nvs_get_u8(nvsHandle, "currentSub", &enableOrDisable) != ESP_OK)
    {
        nvs_close(nvsHandle);
    }
    nvs_close(nvsHandle);
    if (enableOrDisable == 1)
        _startCurrentSubmission();
}

void _currentSubmissionTask()
{
    currentCals = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, VREF_power, currentCals);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(TAG, "Characterized using eFuse Vref");
    }
    else
    {
        ESP_LOGI(TAG, "Characterized using default vref");
    }
    double dprevVal = 0;
    while (true)
    {
        uint32_t ucurrentVal = NodeUtilities_ReadMilliVolts(currentGPIO, 100, currentCals);
        double dcurrentVal = (ucurrentVal / 1000.0) / (0.011 * 60); //equation for current reading
        double diff;
        if (dcurrentVal >= dprevVal)
            diff = dcurrentVal - dprevVal;
        else
            diff = dprevVal - dcurrentVal;
        dprevVal = dcurrentVal;
        //ESP_LOGI(TAG, "The current is %f", dcurrentVal);
        //ESP_LOGI(TAG, "The difference is %f", diff);
        if (diff > 0.200)
        {
            cJSON *currentData = cJSON_CreateObject();
            cJSON_AddNumberToObject(currentData, "current", (truncf(dcurrentVal * 10.0) / 10.0));
            cJSON_AddItemToObject(currentData, "devID", cJSON_CreateString(deviceMACStr));
            cJSON_AddNumberToObject(currentData, "devType", devType);
            char *dataToSend = cJSON_PrintUnformatted(currentData);
            NodeUtilities_PrepareJsonAndSendToRoot(58, 0, dataToSend);
            cJSON_Delete(currentData);
            free(dataToSend);
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void _startCurrentSubmission()
{
    xTaskCreate(_currentSubmissionTask, "_currentSubmissionTask", 4096, NULL, 4, &submissionTaskHandle);
}

void _initPowerNode()
{
    //initialising ADC stuff
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_channel_t adcChannel;
    adcChannel = NodeUtilities_AdcChannelLookup(currentGPIO);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);
}

void SP_Init()
{
    setWiper(256);
    _initLedDriver();
    _initPowerNode();
    _loadVoltageAndPWMValues();
    _loadCurrentSubTask();
}

//##############  BELOW ARE THE COMMANDS   #####################################
//##########################################################################

bool SP_SetVoltage(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SP_SetVoltage Function Called");
    //uint16_t potVal = ceil((((((53500 * 0.8) / (structNodeReceived->dValue - 0.8)) - 75.0) * 256.0) / 10000.0));
    //uint16_t potVal = ceil((((((30000 * 0.8) / (structNodeReceived->dValue - 0.8)) - 75.0) * 256.0) / 10000.0));
    //uint16_t potVal = ceil((766.08/((structNodeReceived->dValue/1.215)-1)));
    double resis = (1.215 / (structNodeReceived->dValue - 1.215)) * 30000;
    uint16_t potVal = ceil((((resis - 75.0) * 256.0) / 10000.0));
    if (potVal >= 245 || structNodeReceived->dValue <= 5)
    //if (potVal >= 256 || structNodeReceived->dValue <= 5)
    {
        potVal = 245;
        //potVal = 256;
        setWiper(potVal);
        //vTaskDelete(NULL);

        //highest - 50volts
    }
    else if (potVal <= 14 || structNodeReceived->dValue >= 59)
    //else if (potVal <= 23 || structNodeReceived->dValue >= 45)
    {
        potVal = 14;
        //potVal = 23;
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
    if (nvs_set_u16(nvsHandle, "potval_SP", potVal) != ESP_OK)
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

bool SP_SetPWMPort0(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SP_SetPWMPort0 Function Called");
    _setPwm((uint8_t)structNodeReceived->dValue, 0);
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_set_u8(nvsHandle, "pwm0", pwm0) != ESP_OK)
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

bool SP_SetPWMPort1(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SP_SetPWMPort1 Function Called");
    _setPwm((uint8_t)structNodeReceived->dValue, 1);
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_set_u8(nvsHandle, "pwm1", pwm1) != ESP_OK)
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

bool SP_SetPWMBothPort(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SP_SetPWMBothPort Function Called");
    _setPwm((uint8_t)structNodeReceived->dValue, 2);
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_set_u8(nvsHandle, "pwm0", pwm0) != ESP_OK)
    {
        nvs_close(nvsHandle);
        ESP_LOGE(TAG, "Log to AWSS");
        return false;
    }
    if (nvs_set_u8(nvsHandle, "pwm1", pwm1) != ESP_OK)
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

bool SP_EnableCurrentSubmission(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SP_EnableCurrentSubmission Function Called");
    uint8_t enableOrDisable = (uint8_t)structNodeReceived->dValue;
    enableOrDisable > 1 ? enableOrDisable = 1 : enableOrDisable;
    if (enableOrDisable == 1)
    {
        //enable submission
        if (submissionTaskHandle != NULL)
            return false;
        _startCurrentSubmission();
    }
    else
    {
        if (submissionTaskHandle != NULL)
        {
            if (currentCals != NULL)
                free(currentCals);
            vTaskDelete(submissionTaskHandle);
            submissionTaskHandle = NULL;
            currentCals = NULL;
        }
    }
    nvs_handle nvsHandle;
    if (nvs_open(nvsStorage, NVS_READWRITE, &nvsHandle) != ESP_OK)
        return false;
    if (nvs_set_u8(nvsHandle, "currentSub", enableOrDisable) != ESP_OK)
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

bool SP_GetVoltage(NodeStruct_t *structNodeReceived)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);
    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, VREF_power, adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "Characterized using eFuse Vref");
    }else{
        ESP_LOGI(TAG, "Characterized using default vref");
    }
    uint32_t V_raw;
    esp_adc_cal_get_voltage(ADC_CHANNEL_3, adc_chars, &V_raw);
    float V_out = ((V_raw/1000.0) * (442000+25500))/25500;
    free(adc_chars);
    cJSON *nodeData = cJSON_CreateObject();
    cJSON_AddItemToObject(nodeData, "devID", cJSON_CreateString(deviceMACStr));
    cJSON_AddNumberToObject(nodeData, "voltage", ((V_out*100)/100));
    char *dataToSend = cJSON_PrintUnformatted(nodeData);
    NodeUtilities_PrepareJsonAndSendToRoot(60, 0, dataToSend);
    cJSON_Delete(nodeData);
    free(dataToSend);
    return true;
}

#endif