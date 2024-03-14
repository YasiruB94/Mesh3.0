/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: machine_state.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in general ESP32 maintenance
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/machine_state.h"

// local variable definitions
static const char *TAG = "machine_state";
TaskHandle_t PowerFailureTask;
TimerHandle_t GW_Availability_Timer;
TimerHandle_t GW_Delayed_Restart_Timer;
bool power_failure_task_completed                   = true;
bool atleast_one_handshake_attempt_recieved         = false;
uint8_t number_of_GW_checks_done_after_HS_attempt   = 0;

/**
 * @brief load the Gateway hardware and firmware versions to variables
 * @return none
 */
void update_GW_version()
{
    // load HW version
    cn_board_info.gw_mcu.hardware_version.major         = GATEWAY_HARDWARE_VERSION_MAJOR;
    cn_board_info.gw_mcu.hardware_version.minor         = GATEWAY_HARDWARE_VERSION_MINOR;
    cn_board_info.gw_mcu.hardware_version.ci            = GATEWAY_HARDWARE_VERSION_CI;

    // load FW version
    cn_board_info.gw_mcu.application_version.major      = GATEWAY_FIRMWARE_VERSION_MAJOR;
    cn_board_info.gw_mcu.application_version.minor      = GATEWAY_FIRMWARE_VERSION_MINOR;
    cn_board_info.gw_mcu.application_version.ci         = GATEWAY_FIRMWARE_VERSION_CI;
    cn_board_info.gw_mcu.application_version.branch_id  = GATEWAY_FIRMWARE_VERSION_BRANCH_ID;
}
/**
 * @brief Begin GW availability check by creating a xTimerCreate function, if it is not already begun
 * @return none
 */
void check_for_GW_availability()
{
    // check if a task is already running
    if (GW_Availability_Timer == NULL)
    {
        ESP_LOGI(TAG, "GW availability check init");
        // init variables
        atleast_one_handshake_attempt_recieved      = false;
        number_of_GW_checks_done_after_HS_attempt   = 0;

        // set proper LED sequence
        LED_assign_task(CNGW_LED_CMD_CONN_PENDING, CNGW_LED_CN);
        // create the task and assign it to a timer
        GW_Availability_Timer = xTimerCreate("check_for_GW_periodically", 5000 / portTICK_RATE_MS, true, NULL, check_for_GW_periodically);
        xTimerStart(GW_Availability_Timer, 0);
    }
}

/**
 * @brief Sequence to trigger GW online state in CN MCU
 * @return none
 */
void check_for_GW_periodically(void *timer)
{
    if (!cn_board_info.cn_config.cn_is_handshake)
    {
        // even after the allowed time, no handshake is performed.
        ESP_LOGI(TAG, "Resetting the GW Online pin");

        // deinit the required pins
        gpio_reset_pin(BDATA_READY);
        gpio_reset_pin(GW_EX1);

        // Notify CN on resetting the GW
        Reset_GW();

        // init the required pins again
        init_output_GPIO();

        // update variables as required
        if(atleast_one_handshake_attempt_recieved)
        {
            number_of_GW_checks_done_after_HS_attempt++;

            // check if the HS is not being successful even though a mainboard is detected
            if(number_of_GW_checks_done_after_HS_attempt > MAX_HS_ATTEMPTS_ALLOWED)
            {
                number_of_GW_checks_done_after_HS_attempt = 0;
                // notify AWS if available
                Send_GW_message_to_AWS(60, 0, "Failed Proper Handshake With Mainboard");
                // restart the GW
                error_handler("check_for_GW_periodically", ESP_FAIL);
            }
        }
    }
}

/**
 * @brief A generic error handler function which displays what the error is. This is a blocking function which after a while makes the ESP to restart
 * @return none
 */
void error_handler(const char* context, esp_err_t reason)
{
    ESP_LOGE(TAG, "Gateway forced to restart. Error in function: %s, Error code: %s", context, esp_err_to_name(reason));
    // set proper LED sequence
    LED_assign_task(CNGW_LED_CMD_ERROR, CNGW_LED_GEN);
    // set a delay as a blocking function
    vTaskDelay(6000 / portTICK_RATE_MS);
    // execute the restart sequence
    ESP_restart_sequence(NULL);
}

/**
 * @brief Non blocking method of a delayed restart, primarily used after OTA session completion
 * @return none
 */
void delayed_ESP_Restart(uint16_t delay_time)
{
    // check if the task is already initiated
    if (GW_Delayed_Restart_Timer == NULL)
    {
        ESP_LOGW(TAG, "Restarting ESP in %d milliseconds ...", delay_time);
        // update the status LED
        LED_assign_task(CNGW_LED_CMD_CONN_PENDING, CNGW_LED_GEN);
        // create the task and assign it to the timer
        GW_Delayed_Restart_Timer = xTimerCreate("ESP_restart_sequence", delay_time / portTICK_RATE_MS, false, NULL, ESP_restart_sequence);
        // start the timer
        xTimerStart(GW_Delayed_Restart_Timer, portMAX_DELAY);
    }
}

/**
 * @brief Sequence to restart the ESP32
 * @return none
 */
void ESP_restart_sequence(void *timer)
{
    ESP_LOGW(TAG, "Restarting now...");
    LED_assign_task(CNGW_LED_CMD_ALL_OFF, CNGW_LED_ALL);
    esp_restart();
}

/**
 * @brief prints out the available heap memory of the machine
 * @return none
 */
void ESP_print_free_heap()
{
#ifdef GATEWAY_DEBUGGING
    ESP_LOGW(TAG, "ESP32 free heap size: %u",esp_get_free_heap_size());
#endif
}
/**
 * @brief builds a string out of the mac addresses
 * @return none
 */

void ESP_mac_to_string(uint8_t mac[], char str[]) {
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8]);
}


/**
 * @brief function triggered by Power loss detection ISR handler.
 * @return none
 */
void task_handle_power_failure(void *pvParameters)
{

    /* //the code which handles the power loss as interrupt method
    power_failure_task_completed            = false;
    cn_board_info.cn_config.cn_is_handshake = false;
    power_failure_handle_sequence();

    while (1)
    {
        if (power_failure_task_completed)
        {
            PowerFailureTask = NULL;
            vTaskDelete(PowerFailureTask);
        }
        vTaskDelay(20 / portTICK_RATE_MS);
    }
    */
   //the code which handles power loss in polling method
    TickType_t start_time, end_time;
    power_failure_task_completed            = false;
    start_time = xTaskGetTickCount();
    end_time = start_time;
   while (1)
   {

    if(gpio_get_level(POWER_DETECT_PIN) == 0)
    {

        if(!power_failure_task_completed)
        {
            ESP_LOGE(TAG, "task_handle_power_failure: Power loss detected ...");
            start_time = xTaskGetTickCount();
            power_failure_handle_sequence();
            power_failure_task_completed = true;
        }
        else
        {
            //comment upto the vtaskdelete if you want to see the time talken for a full depletion
            delayed_ESP_Restart(10000);
            LED_assign_task(CNGW_LED_CMD_ERROR, CNGW_LED_GEN);
            vTaskDelete(NULL);
            end_time = xTaskGetTickCount();
            ESP_LOGI(TAG, "Time after power loss: %d", (end_time - start_time)* portTICK_PERIOD_MS);

        }


    }

    vTaskDelay(20 / portTICK_RATE_MS);
   }

}


/**
 * @brief Sequence of operation to handle a power loss situation.
 * @return none
 */
void power_failure_handle_sequence()
{
    ESP_LOGE(TAG, "Mainboard power failure detected ...");
    Send_GW_message_to_AWS(60, 0, "Mainboard Power Failure");
}
#endif