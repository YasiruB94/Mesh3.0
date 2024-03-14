#include "SIM7080_lib.h"

static const char *TAG = "SIM7080_lib";
SIM7080_t Modem = {0};

/**
 * @brief   Get the SIM context. The context memory space
 *          always exist but member in the memory space may not exist or may be in an invalid state.
 *          when calling this, follow the below method: const SIM7080_t *const mySIMmodule = SIM_Get_Context();
 * @return  A valid memory space for SIM7080s
 */
SIM7080_t *SIM_Get_Context(void)
{ 
    return &Modem;
}

/**
 * @brief  Serach the array for matching characters.
 * @param data          pointer to the information
 * @param data_length   length of the information 
 * @param character     characters to match
 * @return              true if the matching character is found, false if not
 */
esp_err_t SIM7080_Strcmp_Data(uint8_t *data, uint8_t data_length, const char *character)
{
    uint16_t character_size = strlen(character);

    if(data_length < character_size)
    {
        return ESP_FAIL;
    }

    for (uint16_t i = 0; i <= data_length - character_size; i++)
    {
        if(memcmp(data + i, character, character_size) == 0)
        {
            // found the character within the string
            return ESP_OK;
        }
    }

    // did not find the character within the string
    return ESP_FAIL;
}

/**
 * @brief  Initiate the setting up of the modem.

 * @return  ESP_OK if success, else return ESP_FAIL
 */
esp_err_t SIM7080_setup(SIM7080_t *SIM_module)
{
    esp_err_t result = ESP_FAIL;

    // 1. check if the modem is already initialized
    if (Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem already initialized");
        }
        return result;
    }

    // 2. copy the params of the arguement to the global variable
    Modem = *SIM_module;


    // 4. configure UART driver
    uart_config_t uart_config =
        {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    // 5. UART configuration and driver install
    result = uart_param_config(Modem.config.uart_port, &uart_config);
    if (result != ESP_OK)
    {
        return result;
    }
    result = uart_set_pin(Modem.config.uart_port, Modem.config.uart_tx_pin, Modem.config.uart_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (result != ESP_OK)
    {
        return result;
    }
    result = uart_driver_install(Modem.config.uart_port, Modem.config.uart_buffer_size * 2, 0, 0, NULL, 0);
    if (result != ESP_OK)
    {
        return result;
    }

        // 6. create semaphore to share UART resource
    Modem.comm.Semaphore_UART_Resource = xSemaphoreCreateBinary();
    if (Modem.comm.Semaphore_UART_Resource == NULL)
    {
        result = ESP_FAIL;
        return result;
    }
    xSemaphoreGive(Modem.comm.Semaphore_UART_Resource);

    // 5. intialize the UART_Rx queue
    Modem.comm.Recieve_queue = xQueueCreate(Modem.config.queue_length, Modem.config.uart_buffer_size * sizeof(uint8_t));
    if (Modem.comm.Recieve_queue == NULL)
    {
        result = ESP_FAIL;
        return result;
    }

    // 6. begin the UART Receive tasks

    xTaskCreate(SIM7080_Receive_Data, "SIM7080_Receive_Data", 4096, NULL, 10, NULL);

    Modem.intialized = true;

    return result;
}

esp_err_t set_internet_status(bool state)
{
    // 1. check if the Modem is initialized before proceeding furthur
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. update the internet status
    Modem.connected_to_internet = state;

    return ESP_OK;
}

esp_err_t set_log_status(bool print_Tx_info, bool print_Rx_info, bool print_log_info)
{
    // 1. check if the Modem is initialized before proceeding furthur
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. update the log status
    Modem.log.print_Tx_info     = print_Tx_info;
    Modem.log.print_Rx_info     = print_Rx_info;
    Modem.log.print_log_info    = print_log_info;

    return ESP_OK;
}

esp_err_t set_AWS_status(bool state)
{
    // 1. check if the Modem is initialized before proceeding furthur
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. update the internet status
    Modem.connected_to_AWS = state;

    return ESP_OK;
}


/**
 * @brief sequence of writing a file to the SIM internal memory
 * @return ESP_OK if success
 */
esp_err_t SIM7080_FS_write_file(SIM7080_FS_t * File)
{
    // 1. check if the modem is initialized
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. check the file size and check if it is valid
    int16_t file_size = File->end_pos - File->start_pos;
    if (file_size <= 0 ||file_size > Modem.config.uart_buffer_size)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "File size error");
        }
        return ESP_FAIL;
    }
    

    // 3. check the validity of the file name
    if (strlen(File->file_name) <= 0 || strlen(File->file_name) > 50)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "File name size error");
        }
        return ESP_FAIL;
    }

    // 4. create the dynamic memory location
    uint8_t recieve_message[Modem.config.uart_buffer_size];
    char *command_buffer = (char *)malloc((file_size) * sizeof(char));
    if (command_buffer == NULL)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Failed to allocate dynamic memory");
        }
        return ESP_FAIL;
    }



    // 5. (if needed) check if the file already exist
    if (File->check_file_availability)
    {
        memset(command_buffer, 0, file_size * sizeof(char));
        sprintf(command_buffer, "AT+CFSGFIS=%d,\"%s\"", File->directory, File->file_name);
        consume_SIM7080_message((uint8_t *)command_buffer, strlen(command_buffer));


        memset(recieve_message, 0, Modem.config.uart_buffer_size);
        if (xQueueReceive(Modem.comm.Recieve_queue, recieve_message, 1000 / portTICK_RATE_MS) != pdTRUE)
        {
            // if a failure occured, return fail
            if (Modem.log.print_log_info)
            {
                ESP_LOGE(TAG, "failed to receive message from SIM");
            }
            free(command_buffer);
            return ESP_FAIL;
        }
        else
        {
            // got the message, now check if it contains a "OK" message
            if (SIM7080_Strcmp_Data((uint8_t *)recieve_message, sizeof(recieve_message), "OK") == ESP_OK)
            {
                if (Modem.log.print_log_info)
                {
                    ESP_LOGE(TAG, "SIM7080 the specified file already exists");
                }
                free(command_buffer);
                return ESP_FAIL;
            }
        }
    }

    // 6. copy the required data
    memset(command_buffer, 0, file_size * sizeof(char));
    sprintf(command_buffer, "AT+CFSWFILE=%d,\"%s\",%d,%d,%d", File->directory, File->file_name, File->mode, file_size, File->timeout);
    consume_SIM7080_message((uint8_t *)command_buffer, strlen(command_buffer));


    // 7. wait for the SIM to respond
    memset(recieve_message, 0, Modem.config.uart_buffer_size);
    if (xQueueReceive(Modem.comm.Recieve_queue, recieve_message, 1000 / portTICK_RATE_MS) != pdTRUE)
    {
        // if a failure occured, return fail
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "failed to receive message from SIM");
        }
        free(command_buffer);
        return ESP_FAIL;
    }
    else
    {
        // got the message, now check if it contains a "DOWNLOAD" message
        if (SIM7080_Strcmp_Data((uint8_t *)recieve_message, sizeof(recieve_message), "DOWNLOAD") == ESP_FAIL)
        {
            if (Modem.log.print_log_info)
            {
                ESP_LOGE(TAG, "SIM7080 initializing write task failed");
            }
            free(command_buffer);
            return ESP_FAIL;
        }
    }
    // 8. proceed to writing the contents of the file
    if (Modem.log.print_log_info)
    {
        ESP_LOGI(TAG, "SIM7080 writing contents of the file to SIM memory");
    }

    // 9. reset the dynamic memory and copy the contents to it
    memset(command_buffer, 0, file_size * sizeof(char));
    memcpy(command_buffer, File->start_pos, file_size);

    // 10. send the file data to the SIM
    consume_SIM7080_message((uint8_t *)command_buffer, strlen(command_buffer));

    // reset the memory of the recieveData buffer
    memset(recieve_message, 0, Modem.config.uart_buffer_size);

    // wait for the feedback
    if (xQueueReceive(Modem.comm.Recieve_queue, recieve_message, (File->timeout + 500) / portTICK_RATE_MS) != pdTRUE)
    {
        // if a failure occured, return fail
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "failed to receive message from SIM");
        }
        free(command_buffer);
        return ESP_FAIL;
    }
    else
    {
        // got the message, now check if it contains a "OK" message
        if (SIM7080_Strcmp_Data((uint8_t *)recieve_message, sizeof(recieve_message), "OK") == ESP_FAIL)
        {
            if (Modem.log.print_log_info)
            {
                ESP_LOGE(TAG, "SIM7080 write task failed");
            }
            free(command_buffer);
            return ESP_FAIL;
        }
    }

    // task completed successfully. clear resources and return success
    free(command_buffer);
    return ESP_OK;
}

esp_err_t send_msg_receive_interrupt(uint8_t *message, uint16_t transmit_message_size, uint8_t amount_of_commands_expected, uint16_t timeout, uint8_t recieve_message[], uint16_t recieve_message_size)
{
        // 1. check if the Modem is initialized before proceeding furthur
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. prepare the message and consume it to the Tx Queue
    if (consume_SIM7080_message((uint8_t *)message, transmit_message_size))
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "failed to add message to Tx queue");
        }
        return ESP_FAIL;
    }

    // 3. begin task
    SIM7080_msg_rcv_task_params_t param = {0};
    param.total_messages = amount_of_commands_expected;
    param.timeout = timeout;
    param.recv_msg = recieve_message;
    param.recv_msg_size = recieve_message_size;
    param.recv_command_1 = "AT+CGATT";
    param.recv_command_2 = "OK";

    xTaskCreate(send_msg_receive_task, "send_msg_receive_task", 4096, &param, 10, NULL);
    return ESP_OK;
}


//TODO: still have to fix this code
void send_msg_receive_task(void *pvParameters)
{
    SIM7080_msg_rcv_task_params_t *param     = (SIM7080_msg_rcv_task_params_t *)pvParameters;
    uint8_t recieve_message[param->recv_msg_size];
    memset(recieve_message, 0, param->recv_msg_size);
    while (1)
    {

        
        if (xQueuePeek(Modem.comm.Recieve_queue, recieve_message, param->timeout / portTICK_RATE_MS) == pdTRUE)
        {
            // if a message is recieved
            if (SIM7080_Strcmp_Data((uint8_t *)recieve_message, sizeof(recieve_message), param->recv_command_1) == ESP_OK)
            {
                ESP_LOGW(TAG, "Data intended for the current task");
                memset(recieve_message, 0, param->recv_msg_size);
                if (xQueueReceive(Modem.comm.Recieve_queue, recieve_message, 200 / portTICK_RATE_MS) == pdTRUE)
                {
                    if (SIM7080_Strcmp_Data((uint8_t *)recieve_message, sizeof(recieve_message), param->recv_command_2) == ESP_OK)
                    {
                        ESP_LOGW(TAG, "SUCCESS, recieved data: %s", (char*)recieve_message);
                    }
                    else
                    {
                        ESP_LOGE(TAG, "FAIL, recieved data: %s", (char*)recieve_message);
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "failed to xQueueReceive");
                }
            }
            else
            {
                ESP_LOGE(TAG, "test point 1");
            }
        }
        else
        {
            // if a filure occured
            if (Modem.log.print_log_info)
            {
                ESP_LOGE(TAG, "send_msg_receive_task: failed to receive message from SIM");
            }

        }
        vTaskDelay(100 / portTICK_RATE_MS);
        vTaskDelete(NULL);

    }
}

/**
 * @brief  Send a message to the SIM module and wait for the response. If there is no response after the timeout, returns false
 * @param message pointer to the command to be sent
 * @param transmit_message_size  the length of the command to be sent
 * @param amount_of_commands_expected the amount of messages the Modem will respond. the return will be the information of the last message
 * @param timeout  time in milliseconds for timeout to occur
 * @return ESP_OK if success, ESP_FAIL if error
 */
esp_err_t send_msg_receive_polling(uint8_t *message, uint16_t transmit_message_size, uint8_t amount_of_commands_expected, uint32_t timeout)
{
    // 1. check if the Modem is initialized before proceeding furthur
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. check if the Modem is busy executing a previous command. if so, wait a max timeout time for the modem to be free
    if (Modem.is_busy)
    {
        TickType_t start_tick = xTaskGetTickCount();
        TickType_t elapsed_ticks = start_tick;
        while (((elapsed_ticks - start_tick) < (timeout * portTICK_RATE_MS)) && Modem.is_busy)
        {
            elapsed_ticks = xTaskGetTickCount();
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }

    // 3. check if the modem is still busy after waiting (if previously was busy)
    if (Modem.is_busy)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem is busy executing a previous command");
        }
        return ESP_FAIL;
    }

    // 2. set modem status as busy
    Modem.is_busy = true;

    // 2. reset the memory of the recieve_message. the size of this array should be the size expected by the queue, which is the Modem.config.uart_buffer_size
    memset(Modem.comm.recieved_message, 0, Modem.config.uart_buffer_size);

    // 3. prepare the message and consume it to the Tx Queue
    if (consume_SIM7080_message((uint8_t *)message, transmit_message_size) != ESP_OK)
    {
        // if a filure occured, return fail
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "failed to send UART message to SIM");
        }
        return ESP_FAIL;
    }

    // 4. handle the recieved messages from Modem as needed
    for (uint8_t i = 0; i < amount_of_commands_expected; i++)
    {
        if (i == amount_of_commands_expected - 1)
        {
            // this is the last command to wait for
            // wait for a response to arrive from the SIM module. (max waititng time is the timeout provided in the arguements)
            if (xQueueReceive(Modem.comm.Recieve_queue, Modem.comm.recieved_message, timeout / portTICK_RATE_MS) == pdTRUE)
            {
                // if a message is recieved, return success
                Modem.is_busy = false;
                return ESP_OK;
            }
            else
            {
                // if a filure occured, return fail
                if (Modem.log.print_log_info)
                {
                    ESP_LOGE(TAG, "failed to receive message from SIM");
                }

                Modem.is_busy = false;
                return ESP_FAIL;
            }
        }
        else
        {
            // this is not the last message to be expected. a generic timeout of 1 second is applied
            // recieved data is not passed to a buffer
            if (xQueueReceive(Modem.comm.Recieve_queue, Modem.comm.recieved_message, 1000 / portTICK_RATE_MS) != pdTRUE)
            {
                // if a filure occured, return fail
                if (Modem.log.print_log_info)
                {
                    ESP_LOGE(TAG, "failed to receive message from SIM");
                }

                Modem.is_busy = false;
                return ESP_FAIL;
            }

            // reset the memory of the recieveData buffer
            memset(Modem.comm.recieved_message, 0 ,Modem.config.uart_buffer_size);
        }
    }

    // the script is not supposed to reach this position. but for code completion, added a return fail status
    Modem.is_busy = false;
    return ESP_FAIL;
}

/**
 * @brief  checks the validity of a message before adding to the UART_Tx_queue
 * @param message pointer to the beginning of the message
 * @param length  the length of the message to be sent
 * @return ESP_OK if success
 */
esp_err_t consume_SIM7080_message(uint8_t *message, size_t length)
{
    if (!Modem.intialized)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Modem not initialized");
        }
        return ESP_FAIL;
    }

    // 2. check if the length is valid
    if (length >= Modem.config.uart_buffer_size)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Received data size is bigger than what can be handled");
        }

        return ESP_FAIL;
    }

    // 5. write the message to UART and get the number of characters successfully copied
    uint16_t pushed_length = 0;
    if (xSemaphoreTake(Modem.comm.Semaphore_UART_Resource, portMAX_DELAY) == pdTRUE)
    {
        pushed_length = uart_write_bytes(Modem.config.uart_port, (const char *)message, length);
        // 6. add the carriage return to the UART to signal end of message

        uart_write_bytes(Modem.config.uart_port, "\r", 1);

        // 4. Release the semaphore when done
        xSemaphoreGive(Modem.comm.Semaphore_UART_Resource);
    }


    // 8. print data if needed
    if (Modem.log.print_Tx_info)
    {
        ESP_LOGI(TAG, "Tx raw data:\n%s", (char *)message);
    }

    // 9. check if the correct size of data is sent via UART
    if (pushed_length != length)
    {
        if (Modem.log.print_log_info)
        {
            ESP_LOGE(TAG, "Tx Error, all data not pushed to UART");
        }
        return ESP_FAIL;
    }

    return ESP_OK;
}


/**
 * @brief  constantly polls for any incoming data from SIM module and if there are any, sends to process them
 */
void SIM7080_Receive_Data(void *pvParameters)
{
    if(Modem.log.print_log_info)
    {
        ESP_LOGI(TAG, "SIM7080_Receive_Data");
    }

    int16_t recv_data_length = 0;
    uint8_t *recvbuf = (uint8_t *)malloc(Modem.config.uart_buffer_size);
    while (1)
    {
        // 1. Initialize the receiving buffer and set it to all zeros
        
        if(recvbuf == NULL)
        {
            ESP_LOGE(TAG, "Error allocating dynamic memory");
        }
        memset(recvbuf, 0, Modem.config.uart_buffer_size);
        
        // 2. wait for the UART resource to be free
        if (xSemaphoreTake(Modem.comm.Semaphore_UART_Resource, portMAX_DELAY) == pdTRUE)
        {
            // 3. get the length of data in the fifo buffer and copy it to the recvbuf
            recv_data_length = uart_read_bytes(Modem.config.uart_port, recvbuf, Modem.config.uart_buffer_size, 15);

            // 4. Release the semaphore when done
            xSemaphoreGive(Modem.comm.Semaphore_UART_Resource);
        }

        // 5. if there is any data to be read the recv_data_length will not be zero
        if (recv_data_length > 0)
        {    
            esp_err_t add_recv_data_to_Recieve_queue = ESP_OK;
            if (Modem.comm.pre_process_recieved_message != NULL)
            {
                add_recv_data_to_Recieve_queue = Modem.comm.pre_process_recieved_message(recvbuf);
            }

            // 4. add it to the UART_Rx queue if needed
            if (add_recv_data_to_Recieve_queue == ESP_OK)
            {
                if (xQueueSend(Modem.comm.Recieve_queue, recvbuf, 1) != pdTRUE)
                {
                    if (Modem.log.print_log_info)
                    {
                        ESP_LOGE(TAG, "Queue error consumung the recieved message");
                    }
                }
                // print data if needed
                if (Modem.log.print_Rx_info)
                {
                    ESP_LOGI(TAG, "Rx raw data:\n%s", recvbuf);
                }
            }

            // 7. clear out the uart buffer
            uart_flush(Modem.config.uart_port);
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
    // 8. free memory resources
    free(recvbuf);
}
