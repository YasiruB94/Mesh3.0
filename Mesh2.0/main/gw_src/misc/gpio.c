/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: gpio.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in configuraing GPIO pins of the ESP32 according to
 * the requirement
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "gw_includes/gpio.h"

/**
 * @brief initialize generic GPIO input pins. The input pins for communicaiton (SPI, I2C etc) is not handled here
 * @return ESP_OK
 */
esp_err_t init_output_GPIO()
{
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << BDATA_READY | 1ULL << GW_ON | 1ULL << GW_EX1);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    return ESP_OK;
}

/*
// Interrupt service routine for power loss identification
static void power_loss_ISR_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (PowerFailureTask == NULL && power_failure_task_completed && !ota_agent_core_OTA_in_progress)
    {
         xTaskCreate(task_handle_power_failure, "task_handle_power_failure", 3500, NULL, 10, &PowerFailureTask);
    }

    // If the interrupt woke up a higher priority task, yield
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}
*/

#ifdef GATEWAY_SIM7080
esp_err_t init_SIM7080_GPIO()
{
    // pin configuration
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << SIM7080_PULSE);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    Reset_SIM7080();
    return ESP_OK;
}

/**
 * @brief sequence to reset the SIM module
 * @return ESP_OK
 */
esp_err_t Reset_SIM7080()
{
    // trigger sequence
    gpio_set_level(SIM7080_PULSE, 0);
    vTaskDelay(1000 / portTICK_RATE_MS);
    gpio_set_level(SIM7080_PULSE, 1);
    vTaskDelay(1000 / portTICK_RATE_MS);
    gpio_set_level(SIM7080_PULSE, 0);
    vTaskDelay(1000 / portTICK_RATE_MS);
    return ESP_OK;
}
#endif
/**
 * @brief initialize the Power detection pin and initialize the ISR for the pin
 * @return ESP_OK
 */
esp_err_t init_input_power_GPIO()
{

    /* // the code which handles the power loss as interrupt method
    // pin configuration
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << POWER_DETECT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // define the ISR for the pin
    gpio_install_isr_service(0);
    gpio_isr_handler_add(POWER_DETECT_PIN, power_loss_ISR_handler, (void*)POWER_DETECT_PIN);

*/

// the code which handles power loss in polling method
    // pin configuration
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << POWER_DETECT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    xTaskCreate(task_handle_power_failure, "task_handle_power_failure", 5000, NULL, 10, &PowerFailureTask);
    return ESP_OK;
}

/**
 * @brief sequence to trigger a GW reset for CN MCU
 * @return ESP_OK
 */
esp_err_t Reset_GW()
{
    gpio_set_level(GW_ON, 1);
    vTaskDelay(1000 / portTICK_RATE_MS);
    gpio_set_level(GW_ON, 0);

    return ESP_OK;
}




/**
 * @brief sequence to signal that data is ready to be read by CN_MCU
 * THIS IS DEPRECIATED NOW
 * @return ESP_OK
 */
esp_err_t Signal_GW_Of_Sending_Data()
{
    gpio_set_level(GW_ON, 1);
    gpio_set_level(GW_ON, 0);
    ets_delay_us(150);
    gpio_set_level(GW_EX1, 0);
    ets_delay_us(500);
    gpio_set_level(GW_EX1, 1);
    
    return ESP_OK;
}

/**
 * @brief sequence to signal the backward data sequence to be read by CN_MCU
 * @return ESP_OK
 */
esp_err_t Backward_Data_Sequence()
{    
    gpio_set_level(BDATA_READY, 0);
    gpio_set_level(GW_ON, 1);
    gpio_set_level(GW_ON, 1);
    gpio_set_level(GW_ON, 0);
    return ESP_OK;

}

/**
 * @brief sequence to signal the backward data ready sequence to be read by CN_MCU
 * @return ESP_OK
 */
esp_err_t Backward_Data_Ready_Sequence()
{
    gpio_set_level(BDATA_READY, 1);
    gpio_set_level(GW_EX1, 0);
    return ESP_OK;
}

/**
 * @brief sequence to signal that the Tx transmission is complete by GW
 * @return ESP_OK
 */
esp_err_t Tx_Complete_Sequence()
{
    gpio_set_level(GW_EX1, 1);
    return ESP_OK;
}


#endif
