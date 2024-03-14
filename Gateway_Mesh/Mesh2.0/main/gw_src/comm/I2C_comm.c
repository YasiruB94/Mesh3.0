/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: I2C_comm.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: All I2C communication functions used in Crypto Chip and LED driver control
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/I2C_comm.h"
static const char *TAG = "I2C_comm";

/**
 * @brief  initialize the I2C communication. The same bus will be used by crypto chip and the LED driver
 * @return result of the function execution
 */
esp_err_t init_I2C()
{
    ESP_LOGI(TAG, "Initializing I2C communication");
    // 1. initialize and populate variables
    esp_err_t result    = ESP_FAIL;
    I2C_PORT            = I2C_NUM_1;
    i2c_config_t conf   = 
    {
        .mode               = I2C_MODE_MASTER,
        .sda_io_num         = I2C_MASTER_SDA_IO,
        .sda_pullup_en      = GPIO_PULLUP_ENABLE,
        .scl_io_num         = I2C_MASTER_SCL_IO,
        .scl_pullup_en      = GPIO_PULLUP_ENABLE,
        .master.clk_speed   = I2C_MASTER_FREQ_HZ,
    };

    // 2. i2c configuration and driver install
    result = i2c_param_config(I2C_PORT, &conf);
    if(result != ESP_OK)
    {
        return result;
    }
    result = i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if(result != ESP_OK)
    {
        return result;
    }
    // 3. create semaphore to share the i2c resource
    Semaphore_I2C_Resource = xSemaphoreCreateBinary();
    if (Semaphore_I2C_Resource == NULL)
    {
        result = ESP_FAIL;
        return result;
    }

    xSemaphoreGive(Semaphore_I2C_Resource);

    return result;
}

/**
 * @brief  delete the 12c driver to free its resources
 * @return result of the function execution
 */
esp_err_t delete_I2C()
{
    return i2c_driver_delete(I2C_PORT);
}

#endif