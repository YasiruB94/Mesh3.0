/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: status_led.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in controlling the lights of the GW with the 
 * LED driver
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/status_led.h"

#ifdef GATEWAY_DEBUGGING
static bool print_LED_info    = false;
#else
static bool print_LED_info    = false;
#endif

static const char *TAG = "status_led";
GW_LED_t GWLED = {0};
StaticTask_t  xStaticTaskBuffer_LED_Main_Loop_task; 

/**
 * @brief  initialize the LED routines
 * @return result of command
 */
esp_err_t init_status_LEDs()
{
    esp_err_t result = ESP_FAIL; 
    // init the struct to handle LEDs
    GWLED.delay_between_stages                          = LED_DELAY_BETWEEN_STAGES;
    GWLED.current_state                                 = 0;
    GWLED.LED_ARRAY[CNGW_LED_GEN].LED_RED_ADDR          = LED_RED_1;
    GWLED.LED_ARRAY[CNGW_LED_GEN].LED_GREEN_ADDR        = LED_GREEN_1;
    GWLED.LED_ARRAY[CNGW_LED_GEN].LED_BLUE_ADDR         = LED_BLUE_1;
    GWLED.LED_ARRAY[CNGW_LED_GEN].previous_command      = CNGW_LED_CMD_No_Action;
    GWLED.LED_ARRAY[CNGW_LED_GEN].current_command       = CNGW_LED_CMD_No_Action;
    GWLED.LED_ARRAY[CNGW_LED_GEN].momentary_task_busy   = false;

    GWLED.LED_ARRAY[CNGW_LED_CN].LED_RED_ADDR           = LED_RED_2;
    GWLED.LED_ARRAY[CNGW_LED_CN].LED_GREEN_ADDR         = LED_GREEN_2;
    GWLED.LED_ARRAY[CNGW_LED_CN].LED_BLUE_ADDR          = LED_BLUE_2;
    GWLED.LED_ARRAY[CNGW_LED_CN].previous_command       = CNGW_LED_CMD_No_Action;
    GWLED.LED_ARRAY[CNGW_LED_CN].current_command        = CNGW_LED_CMD_No_Action;
    GWLED.LED_ARRAY[CNGW_LED_CN].momentary_task_busy    = false;

    GWLED.LED_ARRAY[CNGW_LED_COMM].LED_RED_ADDR         = LED_RED_3;
    GWLED.LED_ARRAY[CNGW_LED_COMM].LED_GREEN_ADDR       = LED_GREEN_3;
    GWLED.LED_ARRAY[CNGW_LED_COMM].LED_BLUE_ADDR        = LED_BLUE_3;
    GWLED.LED_ARRAY[CNGW_LED_COMM].previous_command     = CNGW_LED_CMD_No_Action;
    GWLED.LED_ARRAY[CNGW_LED_COMM].current_command      = CNGW_LED_CMD_No_Action;
    GWLED.LED_ARRAY[CNGW_LED_COMM].momentary_task_busy  = false;

    // 1. reset the I2C LED driver
    result = PCA9685_Reset();
    if (result != ESP_OK)
    {
        return result;
    }
    // 2. Set the desired frequency for the LED driver
    result = PCA9685_setFrequency(1000);
    if (result != ESP_OK)
    {
        return result;
    }

    // 3. Set Open-Drain mode
    result = PCA9685_Generic_write_i2c_register(MODE2, 0x1B);
    if (result != ESP_OK) {
        return result;
    }


    // 4. turn all attached LED levels to zero
    result = LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_ALL);
    if (result != ESP_OK)
    {
        return result;
    }

    // 5. initiate tasks for each LED
    // Create memory space in PSRAM and assign this memory to a task to handle LED_Main_Loop_task
    StackType_t *psram_buffer_LED_Main_Loop_task = (StackType_t *)heap_caps_malloc(3000 * sizeof(StackType_t), MALLOC_CAP_SPIRAM);
    if (psram_buffer_LED_Main_Loop_task == NULL)
    {
        return ESP_FAIL;
    }

    if (xTaskCreateStatic(LED_Main_Loop_task, "LED_Main_Loop_task", 3000, NULL, 10, psram_buffer_LED_Main_Loop_task, &xStaticTaskBuffer_LED_Main_Loop_task) == NULL)
    {
        return ESP_FAIL;
    }

    return result;
}

/**
 * @brief  Reset the PCA9685
 * @return result of command
 */
esp_err_t PCA9685_Reset(void)
{
    esp_err_t ret = ESP_FAIL;

    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (LED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, MODE1, ACK_CHECK_EN); // 0x0 = "Mode register 1"
        i2c_master_write_byte(cmd, 0x80, ACK_CHECK_EN);  // 0x80 = "Reset"
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, PCA9685_RESPONSE_TIMEOUT_GENERIC / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }
    return ret;
}

/**
 * @brief Write two 16 bit values to the same register on an i2c device
 * @param[in]  regaddr   The register address
 * @param[in]  valueOn   The value on
 * @param[in]  valueOff  The value off
 * @return result of command
 */
esp_err_t PCA9685_Generic_write_i2c_register_two_words(uint8_t regaddr, uint16_t valueOn, uint16_t valueOff)
{
    esp_err_t ret = ESP_FAIL;
    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (LED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, regaddr, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, valueOn & 0xff, ACK_VAL);
        i2c_master_write_byte(cmd, valueOn >> 8, NACK_VAL);
        i2c_master_write_byte(cmd, valueOff & 0xff, ACK_VAL);
        i2c_master_write_byte(cmd, valueOff >> 8, NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, PCA9685_RESPONSE_TIMEOUT_GENERIC / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }
    return ret;
}

/**
 * @brief Write a 16 bit value to a register on an i2c device
 * @param[in]  regaddr  The register address
 * @param[in]  value    The value
 * @return result of command
 */
esp_err_t PCA9685_Generic_write_i2c_register_word(uint8_t regaddr, uint16_t value)
{
    esp_err_t ret = ESP_FAIL;
    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (LED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, regaddr, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, value & 0xff, ACK_VAL);
        i2c_master_write_byte(cmd, value >> 8, NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, PCA9685_RESPONSE_TIMEOUT_GENERIC / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }
    return ret;
}


/**
 * @brief Write a 8 bit value to a register on an i2c device
 * @param[in]  regaddr  The register address
 * @param[in]  value    The value
 * @return result of command
 */
esp_err_t PCA9685_Generic_write_i2c_register(uint8_t regaddr, uint8_t value)
{
    esp_err_t ret = ESP_FAIL;

    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (LED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, regaddr, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, value, NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, PCA9685_RESPONSE_TIMEOUT_GENERIC / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }
    return ret;
}

/**
 * @brief Read two 8 bit values from the same register on an i2c device
 * @param[in]  regaddr  The register address
 * @param      valueA   The first value
 * @param      valueB   The second value
 * @return result of command
 */
esp_err_t PCA9685_Generic_read_two_i2c_register(uint8_t regaddr, uint8_t *valueA, uint8_t *valueB)
{
    esp_err_t ret = ESP_OK;
    i2c_cmd_handle_t cmd;
    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (LED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, regaddr, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, PCA9685_RESPONSE_TIMEOUT_GENERIC / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (xSemaphoreTake(Semaphore_I2C_Resource, portMAX_DELAY) == pdTRUE)
    {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, LED_I2C_ADDRESS << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
        i2c_master_read_byte(cmd, valueA, ACK_VAL);
        i2c_master_read_byte(cmd, valueB, NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_PORT, cmd, PCA9685_RESPONSE_TIMEOUT_GENERIC / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        // Release the semaphore when done
        xSemaphoreGive(Semaphore_I2C_Resource);
    }
    return ret;
}

/**
 * @brief Read a 16 bit value from a register on an i2c decivde
 * @param[in]  regaddr  The register address
 * @param      value    The value
 * @return result of command
 */
esp_err_t PCA9685_Generic_read_i2c_register_word(uint8_t regaddr, uint16_t* value)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t valueA;
    uint8_t valueB;

    ret = PCA9685_Generic_read_two_i2c_register(regaddr, &valueA, &valueB);
    if (ret != ESP_OK) {
        return ret;
    }
    *value = (valueB << 8) | valueA;
    return ret;
}

/**
 * @brief Sets the frequency of PCA9685
 * @param[in]  freq  The frequency
 * @return result of command
 */
esp_err_t PCA9685_setFrequency(uint16_t freq)
{
    esp_err_t ret = ESP_FAIL;

    // 1. Send to sleep
    ret = PCA9685_Generic_write_i2c_register(MODE1, 0x10);
    if (ret != ESP_OK) {
        return ret;
    }

    // 2. Set prescaler
    // 3. calculation on page 25 of datasheet
    uint8_t prescale_val = round((CLOCK_FREQ / 4096 / (0.9*freq)) - 1+0.5);
    ret = PCA9685_Generic_write_i2c_register(PRE_SCALE, prescale_val);
    if (ret != ESP_OK) {
        return ret;
    }

    // 4. reset again
    PCA9685_Reset();

    // 5. Send to sleep again
    ret = PCA9685_Generic_write_i2c_register(MODE1, 0x10);
    if (ret != ESP_OK) {
        return ret;
    }

    // 6. wait
    vTaskDelay(5/portTICK_PERIOD_MS);

    // 7. Write 0xa0 for auto increment LED0_x after received cmd
    ret = PCA9685_Generic_write_i2c_register(MODE1, 0xa0);

    return ret;
}

/**
 * @brief Sets the pwm of the pin
 * @param[in]  num   The pin number
 * @param[in]  on    On time
 * @param[in]  off   Off time
 * @return result of command
 */
esp_err_t PCA9685_Set_PWM(uint8_t num, uint16_t on, uint16_t off)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t pinAddress = LED0_ON_L + LED_MULTIPLYER * num;
    ret = PCA9685_Generic_write_i2c_register_two_words(pinAddress & 0xff, on, off);

    return ret;
}


/**
 * @brief  Turn all LEDs off
 * @return result of command
 */
esp_err_t PCA9685_Turn_all_LEDs_off(void)
{
    esp_err_t ret = ESP_FAIL;
    if(print_LED_info)
    {
        ESP_LOGI(TAG, "Turn all LEDs off");
    }
    // to shut down ALL channels
    /*
    uint16_t valueOn = 0;
    uint16_t valueOff = 4096;
    ret = PCA9685_Generic_write_i2c_register_two_words(ALLLED_ON_L, valueOn, valueOff);
    */

    // to shut down ONLY LED channels
    ret = ESP_OK;
    PCA9685_Set_PWM(LED_RED_1,      0, LED_MAX_VAL - 0);
    PCA9685_Set_PWM(LED_RED_2,      0, LED_MAX_VAL - 0);
    PCA9685_Set_PWM(LED_RED_3,      0, LED_MAX_VAL - 0);

    PCA9685_Set_PWM(LED_GREEN_1,    0, LED_MAX_VAL - 0);
    PCA9685_Set_PWM(LED_GREEN_2,    0, LED_MAX_VAL - 0);
    PCA9685_Set_PWM(LED_GREEN_3,    0, LED_MAX_VAL - 0);

    PCA9685_Set_PWM(LED_BLUE_1,     0, LED_MAX_VAL - 0);
    PCA9685_Set_PWM(LED_BLUE_2,     0, LED_MAX_VAL - 0);
    PCA9685_Set_PWM(LED_BLUE_3,     0, LED_MAX_VAL - 0);

    return ret;
}


static void LED_GEN_Restore_previous_state(TimerHandle_t xTimer)
{
    GWLED.LED_ARRAY[CNGW_LED_GEN].momentary_task_busy = false;
    LED_assign_task(GWLED.LED_ARRAY[CNGW_LED_GEN].previous_command, CNGW_LED_GEN);
    xTimerDelete(xTimer, 0);
}
static void LED_CN_Restore_previous_state(TimerHandle_t xTimer)
{
    GWLED.LED_ARRAY[CNGW_LED_CN].momentary_task_busy = false;
    LED_assign_task(GWLED.LED_ARRAY[CNGW_LED_CN].previous_command, CNGW_LED_CN);
    xTimerDelete(xTimer, 0);
}
static void LED_COMM_Restore_previous_state(TimerHandle_t xTimer)
{
    GWLED.LED_ARRAY[CNGW_LED_COMM].momentary_task_busy = false;
    LED_assign_task(GWLED.LED_ARRAY[CNGW_LED_COMM].previous_command, CNGW_LED_COMM);
    xTimerDelete(xTimer, 0);
}

esp_err_t LED_change_task_momentarily(CNGW_LED_Command command, CNGW_LED_Type target, uint16_t delay)
{
    if (target == CNGW_LED_NONE || target == CNGW_LED_ALL)
    {
        return ESP_FAIL;
    }

    LED_assign_task(command, target);

    switch (target)
    {
    case CNGW_LED_GEN:
    {
        if (GWLED.LED_ARRAY[CNGW_LED_GEN].momentary_task_busy == false)
        {
            GWLED.LED_ARRAY[CNGW_LED_GEN].momentary_task_busy = true;
            TimerHandle_t myTimer = xTimerCreate("myTimer", delay / portTICK_RATE_MS, pdFALSE, 0, LED_GEN_Restore_previous_state);
            xTimerStart(myTimer, 0);
        }
    }
    break;
    case CNGW_LED_CN:
    {
        if (GWLED.LED_ARRAY[CNGW_LED_CN].momentary_task_busy == false)
        {
            GWLED.LED_ARRAY[CNGW_LED_CN].momentary_task_busy = true;
            TimerHandle_t myTimer = xTimerCreate("myTimer", delay / portTICK_RATE_MS, pdFALSE, 0, LED_CN_Restore_previous_state);
            xTimerStart(myTimer, 0);
        }
    }
    break;
    case CNGW_LED_COMM:
    {
        if (GWLED.LED_ARRAY[CNGW_LED_COMM].momentary_task_busy == false)
        {
            GWLED.LED_ARRAY[CNGW_LED_COMM].momentary_task_busy = true;
            TimerHandle_t myTimer = xTimerCreate("myTimer", delay / portTICK_RATE_MS, pdFALSE, 0, LED_COMM_Restore_previous_state);
            xTimerStart(myTimer, 0);
        }
    }
    break;
    default:
    {
    }
    break;
    }

    return ESP_OK;
}

void LED_Main_Loop_task(void *pvParameters)
{
    GWLED.current_state = 0;
    PCA9685_Turn_all_LEDs_off();

    while (1)
    {
        if (GWLED.current_state == LED_MAX_STAGES)
        {
            GWLED.current_state = 0;
        }

        for (uint8_t LED_NUM = 1; LED_NUM < CNGW_LED_ALL; LED_NUM++)
        {
            // LED RED
            if (GWLED.LED_ARRAY[LED_NUM].RED[GWLED.current_state] != GWLED.LED_ARRAY[LED_NUM].RED_P)
            {
                GWLED.LED_ARRAY[LED_NUM].RED_P = GWLED.LED_ARRAY[LED_NUM].RED[GWLED.current_state];
                if (GWLED.LED_ARRAY[LED_NUM].RED[GWLED.current_state] == true)
                {
                    if (PCA9685_Set_PWM(GWLED.LED_ARRAY[LED_NUM].LED_RED_ADDR, LED_RED_MAX_VAL, LED_MAX_VAL - LED_RED_MAX_VAL) != ESP_OK)
                    {
                        GWLED.LED_ARRAY[LED_NUM].RED_P = !GWLED.LED_ARRAY[LED_NUM].RED_P;
                    }
                }
                else
                {
                    if (PCA9685_Set_PWM(GWLED.LED_ARRAY[LED_NUM].LED_RED_ADDR, 0, LED_MAX_VAL - 0) != ESP_OK)
                    {
                        GWLED.LED_ARRAY[LED_NUM].RED_P = !GWLED.LED_ARRAY[LED_NUM].RED_P;
                    }
                }
            }

            // LED GREEN
            if (GWLED.LED_ARRAY[LED_NUM].GREEN[GWLED.current_state] != GWLED.LED_ARRAY[LED_NUM].GREEN_P)
            {
                GWLED.LED_ARRAY[LED_NUM].GREEN_P = GWLED.LED_ARRAY[LED_NUM].GREEN[GWLED.current_state];
                if (GWLED.LED_ARRAY[LED_NUM].GREEN[GWLED.current_state] == true)
                {
                    if (PCA9685_Set_PWM(GWLED.LED_ARRAY[LED_NUM].LED_GREEN_ADDR, LED_GREEN_MAX_VAL, LED_MAX_VAL - LED_GREEN_MAX_VAL) != ESP_OK)
                    {
                        GWLED.LED_ARRAY[LED_NUM].GREEN_P = !GWLED.LED_ARRAY[LED_NUM].GREEN_P;
                    }
                }
                else
                {
                    if (PCA9685_Set_PWM(GWLED.LED_ARRAY[LED_NUM].LED_GREEN_ADDR, 0, LED_MAX_VAL - 0) != ESP_OK)
                    {
                        GWLED.LED_ARRAY[LED_NUM].GREEN_P = !GWLED.LED_ARRAY[LED_NUM].GREEN_P;
                    }
                }
            }

            // LED BLUE
            if (GWLED.LED_ARRAY[LED_NUM].BLUE[GWLED.current_state] != GWLED.LED_ARRAY[LED_NUM].BLUE_P)
            {
                GWLED.LED_ARRAY[LED_NUM].BLUE_P = GWLED.LED_ARRAY[LED_NUM].BLUE[GWLED.current_state];
                if (GWLED.LED_ARRAY[LED_NUM].BLUE[GWLED.current_state] == true)
                {
                    if (PCA9685_Set_PWM(GWLED.LED_ARRAY[LED_NUM].LED_BLUE_ADDR, LED_BLUE_MAX_VAL, LED_MAX_VAL - LED_BLUE_MAX_VAL) != ESP_OK)
                    {
                        GWLED.LED_ARRAY[LED_NUM].BLUE_P = !GWLED.LED_ARRAY[LED_NUM].BLUE_P;
                    }
                }
                else
                {
                    if (PCA9685_Set_PWM(GWLED.LED_ARRAY[LED_NUM].LED_BLUE_ADDR, 0, LED_MAX_VAL - 0) != ESP_OK)
                    {
                        GWLED.LED_ARRAY[LED_NUM].BLUE_P = !GWLED.LED_ARRAY[LED_NUM].BLUE_P;
                    }
                }
            }
        }
        
        GWLED.current_state++;
        vTaskDelay(GWLED.delay_between_stages / portTICK_RATE_MS);
    }
}

esp_err_t LED_assign_task(CNGW_LED_Command command, CNGW_LED_Type target)
{
    bool all_commands_executed      = false;
    CNGW_LED_Type LED_TARGET        = CNGW_LED_NONE;
    GWLED.current_state             = 0;

    while (!all_commands_executed)
    {
        if (target == CNGW_LED_ALL)
        {
            LED_TARGET++;
            if (LED_TARGET == CNGW_LED_COMM)
            {
                all_commands_executed = true;
            }
            else
            {
                all_commands_executed = false;
            }
        }
        else
        {
            all_commands_executed   = true;
            LED_TARGET              = target;
        }
        if(GWLED.LED_ARRAY[LED_TARGET].momentary_task_busy == false)
        {
            GWLED.LED_ARRAY[LED_TARGET].previous_command    = GWLED.LED_ARRAY[LED_TARGET].current_command;
            GWLED.LED_ARRAY[LED_TARGET].current_command     = command;

            switch (command)
            {
            case CNGW_LED_CMD_ALL_OFF:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_FW_UPDATE_PRE_PREPARATION:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_FW_UPDATE:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = true;
            }
            break;
            case CNGW_LED_CMD_CONN_PENDING:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_IDLE:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = true;
            }
            break;
            case CNGW_LED_CMD_BUSY:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = true;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_ERROR:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = true;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_CONN_STAGE_01:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_CONN_STAGE_02:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = false;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            case CNGW_LED_CMD_GET_CONFIG_INFO:
            {
                GWLED.LED_ARRAY[LED_TARGET].RED[0]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[1]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[2]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[3]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[4]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[5]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[6]      = false;
                GWLED.LED_ARRAY[LED_TARGET].RED[7]      = false;

                GWLED.LED_ARRAY[LED_TARGET].GREEN[0]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[1]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[2]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[3]    = false;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[4]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[5]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[6]    = true;
                GWLED.LED_ARRAY[LED_TARGET].GREEN[7]    = true;

                GWLED.LED_ARRAY[LED_TARGET].BLUE[0]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[1]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[2]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[3]     = true;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[4]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[5]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[6]     = false;
                GWLED.LED_ARRAY[LED_TARGET].BLUE[7]     = false;
            }
            break;
            default:
            {
            }
            break;
            }
        }
        
    };

    return ESP_OK;
}

#endif