/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: SIM7080.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: link between SIM7080 module and the GW
 * 
 * NOTE: The component library SIM7080_lib.c is required
 ******************************************************************************
 *
 ******************************************************************************
 */
#ifdef GATEWAY_SIM7080
#include "gw_includes/SIM7080.h"
static const char *TAG = "SIM7080";
#ifdef GW_DEBUGGING
static bool Print_Info = true;
#else
static bool Print_Info = false;
#endif

// local variables
SIM7080_t GW_SIM7080 = {0};
TaskHandle_t SIM7080_internet_handler;
TimerHandle_t SIM7080_internet_reconnection_handler;
QueueHandle_t SIM7080_AWS_Tx_queue;
QueueHandle_t SIM7080_AWS_Rx_queue;
uint8_t recieved_data_from_SIM7080[UART_BUF_SIZE];
bool temp_block_SIM7080_msg_preprocessor = false;
int SIM7080_OTA_data_recieved_length     = 0;
char *substring_end;
char *substring_start;

/**
 * @brief  initialize the communication for SIM7080 module. The communication mode is UART
 * @return result of the function execution
 */
esp_err_t init_SIM7080()
{
    esp_err_t result = ESP_FAIL;

    ESP_LOGI(TAG, "Initializing SIM7080");

    // 1. initialize and do the pre-reset of the SIM module
    init_SIM7080_GPIO();

    // 2. initialize and populate variables
    GW_SIM7080.config.uart_tx_pin       = SIM7080_TX;
    GW_SIM7080.config.uart_rx_pin       = SIM7080_RX;
    GW_SIM7080.config.wakeup_pin        = SIM7080_PULSE;
    GW_SIM7080.config.uart_buffer_size  = UART_BUF_SIZE;
    GW_SIM7080.config.queue_length      = UART_QUEUE_LENGTH;
    GW_SIM7080.config.uart_port         = SIM7080_UART_PORT;
    GW_SIM7080.log.print_Tx_info        = Print_Info;
    GW_SIM7080.log.print_Rx_info        = Print_Info;
    GW_SIM7080.log.print_log_info       = Print_Info;
    GW_SIM7080.comm.recieved_message    = recieved_data_from_SIM7080;
    GW_SIM7080.comm.pre_process_recieved_message = SIM7080_msg_preprocessor;
    GW_SIM7080.is_busy                  = false;

    result = SIM7080_setup(&GW_SIM7080);
    if(result !=ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting up SIM7080");
        return ESP_FAIL;
    }

    // 3. copy the context of the Modem to the local instance
    GW_SIM7080 = *SIM_Get_Context();

    // 4. create a transmit queue with maximum of 10 commands
    SIM7080_AWS_Tx_queue = xQueueCreate(10, sizeof(char*));
    if(SIM7080_AWS_Tx_queue == NULL)
    {
        ESP_LOGE(TAG, "Could not create SIM AWS Tx queue");
        return ESP_FAIL;
    }

    // 5. created a recieve queue with maxumum of 5 commands
    SIM7080_AWS_Rx_queue = xQueueCreate(5, sizeof(char *));
    if (SIM7080_AWS_Rx_queue == NULL)
    {
        ESP_LOGE(TAG, "Could not create SIM AWS Rx queue");
        return ESP_FAIL;
    }

    // 6. create task to execute commands
    xTaskCreate(SIM7080_CommandExecutionTask, "SIM7080_CommandExecutionTask", 5120, NULL, 4, NULL);

    // 7. create tasks to send messages to aws
    xTaskCreate(SIM7080_AWS_Tx_task, "SIM7080_AWS_Tx_task", 3000, NULL, 5, NULL);

    return result;
}


/**
 * @brief  retry connecting to internet after a given period of time. first checks if there is already an attempt being made.
 * @return ESP_OK
 */
esp_err_t SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(uint16_t delay_time)
{
    if(SIM7080_internet_reconnection_handler == NULL)
    {
        ESP_LOGW(TAG, "Reattempting internet and AWS connection");

        SIM7080_internet_reconnection_handler = xTimerCreate("SIM7080_Connect_to_Internet_And_AWS", delay_time / portTICK_RATE_MS, false, NULL, SIM7080_Connect_to_Internet_And_AWS);
        xTimerStart(SIM7080_internet_reconnection_handler, portMAX_DELAY);
    }
    

    return ESP_OK;
}

/**
 * @brief  function which begins the task to get SIM7080 connected to internet. first checks if there is already an attempt being made.
 */
void SIM7080_Connect_to_Internet_And_AWS(void *timer)
{
    if (SIM7080_internet_handler == NULL)
    {
        xTaskCreate(get_SIM_connected_to_internet, "get_SIM_connected_to_internet", 3500, NULL, 12, &SIM7080_internet_handler);
    }
    else
    {
        ESP_LOGE(TAG, "SIM already trying to connect to Internet and AWS");
    }
}

/**
 * @brief       decodes and filter out incoming information from SIM module. The number of "\r\n" is 2
 * @param data  pointer to the information
 */
esp_err_t SIM7080_Process_Type_01_Data(uint8_t *data)
{
    // 1. initialize function specific variables
    uint8_t number_br_occurances = 0;
    uint8_t br_positions[4] = {0};

    // 2. identify the position of the begining of the message
    char *start = (char *)data;

    // 3. from the beginning to the end of the message, increment the position in steps of 2 and calculate the number of times "\r\n" occurs.
    // record this number as the number of br occurances. Also, record these indexes as well
    while ((start = strstr(start, "\r\n")) != NULL)
    {
        br_positions[number_br_occurances] = (int)(start - (char *)data);
        number_br_occurances++;
        // for safety, stop the iteration at 4 max occurances
        if (number_br_occurances == 4)
        {
            break;
        }
        start += 2;
    }

    // 4. if there were no "\r\n" occurances, then set the number of occurances to zero
    if (br_positions[0] == 0)
    {
        number_br_occurances = 0;
    }

    // 5. make sure the recieved data has only 2 /r/n.
    if (number_br_occurances != 2)
    {
        return ESP_FAIL;
    }

    // 5. handle decoding according to the number of "\r\n" occurances found

    // initialize and assign values to header, response, status variables
    uint8_t header_start_pos = 0;
    uint8_t header_end_pos = br_positions[0];
    uint8_t status_start_pos = br_positions[0] + 2;
    uint8_t status_end_pos = br_positions[1];
    uint8_t *header = (uint8_t *)malloc((header_end_pos - header_start_pos) * sizeof(uint8_t));
    uint8_t *status = (uint8_t *)malloc((status_end_pos - status_start_pos) * sizeof(uint8_t));

    // check for successful memory allocation
    if (header == NULL || status == NULL)
    {
        ESP_LOGE(TAG, "Error allocating dynamic memory");
        free(header);
        free(status);
        return ESP_FAIL;
    }

    // copy data to dynamic memory
    memcpy(header, &data[header_start_pos], (header_end_pos - header_start_pos));
    memcpy(status, &data[status_start_pos], (status_end_pos - status_start_pos));

    // print data if needed
    if (Print_Info)
    {
        printf("Rx data below:\nHEADER:\t\t");
        for (uint8_t i = 0; i < (header_end_pos - header_start_pos); i++)
        {
            printf("%c", header[i]);
        }

        printf("\nSTATUS:\t\t");
        for (uint8_t i = 0; i < (status_end_pos - status_start_pos); i++)
        {
            printf("%c", status[i]);
        }
        printf("\n");
    }

    // checking of the response from SIM module is an "OK" command
    // before checking, make sure the length of the status array is atleast 2 bytes
    esp_err_t result = ESP_FAIL;
    if (status_end_pos - status_start_pos >= 2)
    {
        // the uint8_t values of ASCII "OK" is 79 and 75. therefore comparing the integer values directly
        if (status[0] == 79 && status[1] == 75)
        {
            result = ESP_OK;
        }
    }

    // free memory resources
    free(header);
    free(status);

    return result;
}

/**
 * @brief        decodes and filter out incoming information from SIM module. The number of "\r\n" is 4
 * @param data   pointer to the information
 */
esp_err_t SIM7080_Process_Type_02_Data(uint8_t *data)
{
    // 1. initialize function specific variables
    uint8_t number_br_occurances = 0;
    uint8_t br_positions[4] = {0};

    // 2. identify the position of the begining of the message
    char *start = (char *)data;

    // 3. from the beginning to the end of the message, increment the position in steps of 2 and calculate the number of times "\r\n" occurs.
    // record this number as the number of br occurances. Also, record these indexes as well
    while ((start = strstr(start, "\r\n")) != NULL)
    {
        br_positions[number_br_occurances] = (int)(start - (char *)data);
        number_br_occurances++;
        // for safety, stop the iteration at 4 max occurances
        if (number_br_occurances == 4)
        {
            break;
        }
        start += 2;
    }

    // 4. if there were no "\r\n" occurances, then set the number of occurances to zero
    if (br_positions[0] == 0)
    {
        number_br_occurances = 0;
    }

    // 5. make sure the recieved data has only 4 /r/n.
    if (number_br_occurances != 4)
    {
        return ESP_FAIL;
    }

    // 5. handle decoding according to the number of "\r\n" occurances found

    // initialize and assign values to header, response, status variables
    uint8_t header_start_pos = 0;
    uint8_t header_end_pos = br_positions[0];
    uint8_t response_start_pos = br_positions[0] + 2;
    uint8_t response_end_pos = br_positions[1];
    uint8_t status_start_pos = br_positions[2] + 2;
    uint8_t status_end_pos = br_positions[3];
    uint8_t *header = (uint8_t *)malloc((header_end_pos - header_start_pos) * sizeof(uint8_t));
    uint8_t *response = (uint8_t *)malloc((response_end_pos - response_start_pos) * sizeof(uint8_t));
    uint8_t *status = (uint8_t *)malloc((status_end_pos - status_start_pos) * sizeof(uint8_t));

    // check for successful memory allocation
    if (header == NULL || response == NULL || status == NULL)
    {
        ESP_LOGE(TAG, "Error allocating dynamic memory");
        free(header);
        free(response);
        free(status);
        return ESP_FAIL;
    }

    // copy data to dynamic memory
    memcpy(header, &data[header_start_pos], (header_end_pos - header_start_pos));
    memcpy(response, &data[response_start_pos], (response_end_pos - response_start_pos));
    memcpy(status, &data[status_start_pos], (status_end_pos - status_start_pos));

    // print data if needed
    if (Print_Info)
    {
        printf("Rx data below:\nHEADER:\t\t");
        for (uint8_t i = 0; i < (header_end_pos - header_start_pos); i++)
        {
            printf("%c", header[i]);
        }
        printf("\nRESPONSE:\t");
        for (uint8_t i = 0; i < (response_end_pos - response_start_pos); i++)
        {
            printf("%c", response[i]);
        }
        printf("\nSTATUS:\t\t");
        for (uint8_t i = 0; i < (status_end_pos - status_start_pos); i++)
        {
            printf("%c", status[i]);
        }
        printf("\n");
    }

    // checking of the response from SIM module is an "OK" command
    // before checking, make sure the length of the status array is atleast 2 bytes
    esp_err_t result = ESP_FAIL;
    if (status_end_pos - status_start_pos >= 2)
    {
        // the uint8_t values of ASCII "OK" is 79 and 75. therefore comparing the integer values directly
        if (status[0] == 79 && status[1] == 75)
        {
            result = ESP_OK;
        }
    }

    // free memory resources
    free(header);
    free(response);
    free(status);

    return result;
}



/**
 * @brief  consume messages comming from AWS to validate to be sent to SIM module
 * if the expected number of commands is 0, success message sent to AWS
 * if the expected number of commands is > 0, max wait time for a successful response is 30 seconds for EACH successive command.
 * @return ESP_OK
 */
esp_err_t Execute_AT_CMD(char *command, uint8_t num_commands)
{
    esp_err_t status = ESP_FAIL;
    LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_COMM, LED_CHANGE_MOMENTARY_DURATION);
    if (num_commands == 0)
    {   
        consume_SIM7080_message((uint8_t *)command, strlen(command));
        Send_GW_message_to_AWS(64, 0, "Command sent to SIM");
    }
    else
    {
        status = send_msg_receive_polling((uint8_t *)command, strlen(command), num_commands, 30000);
        if (status == ESP_OK)
        {
            ESP_LOGW(TAG, "Recieved: \n%s", (char *)recieved_data_from_SIM7080);
            Send_GW_message_to_AWS(64, 0, (char *)recieved_data_from_SIM7080);
        }
        else
        {
            ESP_LOGE(TAG, "failed to receive message from SIM");
            Send_GW_message_to_AWS(65, 0, "failed to receive message from SIM");
        }
    }

    return ESP_OK;
}

/**
 * @brief  Uploads the Certificates to the SIM module
 * @return ESP_OK
 */
esp_err_t Send_Certs_To_SIM()
{
    // 1. define and load data to local variables
    esp_err_t status                      = ESP_FAIL;
    uint8_t connection_attempts           = 0;
    const char * File_locations_Start[]   = {(const char *)aws_root_ca_sim_pem_start,   (const char *)certificate_pem_crt_start,    (const char *)private_pem_key_start};
    const char * File_locations_End[]     = {(const char *)aws_root_ca_sim_pem_end,     (const char *)certificate_pem_crt_end,      (const char *)private_pem_key_end};
    const char * File_name[]              = {"rootCA.pem",                              "deviceCert.crt",                           "devicePKey.key"};
    SIM7080_FS_t CertFile = {0};

    // 2. sometimes, the SIM7080 will not respond for the first few messages. therefore, in a loop, send AT check till success or connection_attempts exceeds 20
    while (status == ESP_FAIL && connection_attempts < 20)
    {
        if(send_msg_receive_polling((uint8_t *)ATcmdC_AT, strlen(ATcmdC_AT), 1, 1000) == ESP_OK)
        {
            // message recieved, now check if it is decodable properly
            status = SIM7080_Process_Type_01_Data((uint8_t *)recieved_data_from_SIM7080);
        }

        connection_attempts ++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if(status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to recieve message from SIM7080");
        return status;
    }

    // 3. initialize the FS in the SIM module 
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSINIT, strlen(ATcmdC_CFSINIT), 1, 1000);
    if(status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to recieve message from SIM7080");
        return status;
    }

    // 4. for each type of file needed to be uploaded, loop
    for (uint8_t i = 0; i < 3; i++)
    {
        // load information to the struct
        CertFile.directory                  = CUSTOMER;
        CertFile.file_name                  = File_name[i];
        CertFile.start_pos                  = File_locations_Start[i];
        CertFile.end_pos                    = File_locations_End[i];
        CertFile.mode                       = WRITE_DATA_FROM_BEGINNING;
        CertFile.timeout                    = 3000;
        CertFile.check_file_availability    = false;

        // write the file and check the result
        status = SIM7080_FS_write_file(&CertFile);
        if (status != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to write the certificate %s to the SIM module", File_name[i]);
            return status;
        }
    }

    // 5. deinit  the FS in the SIM module
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSTERM, strlen(ATcmdC_CFSTERM), 1, 1000);
    if(status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to recieve message from SIM7080");
    }

    return status;
}

/**
 * @brief  clear out the queue which handles the recieved messages from SIM7080
 * @return status 
 */
esp_err_t SIM7080_Clear_Queues()
{
    GW_SIM7080 = *SIM_Get_Context();
    // 1. check if SIM7080 is intialized
    if (!GW_SIM7080.intialized)
    {
        ESP_LOGE(TAG, "SIM not initialized");
        return ESP_FAIL;
    }

    
    BaseType_t queueStatus;
    BaseType_t queueEmpty = pdFALSE;
    uint8_t recieve_message[GW_SIM7080.config.uart_buffer_size];
    // 2. clear  recieve queue
    while (queueEmpty == pdFALSE)
    {
        queueStatus = xQueueReceive(GW_SIM7080.comm.Recieve_queue, recieve_message, 0);

        if(queueStatus == pdFALSE)
        {
            queueEmpty = pdTRUE;
        }
    }

    return ESP_OK;
}

// The below functions will only be needed when the GW is acting as only SIM7080 driven unit.
// (if IPNODE is defined, the OTA information must be sent via IPNODE)
#ifndef IPNODE

/**
 * @brief Handles CENCE OTA FW
 *  checks for FW validity, connects to URL and download the FW to SIM7080
 *  copies the FW to the ESP32 memory temporarily
 *  sends the FW to Mainboard
 * @param structRootWrite[in] URL to download the fW from
 */
void SIM7080_UpgradeNodeCenceFirmware(NodeStruct_t *structNodeReceived)
{
    // 1. Initialize and reset variables
    esp_err_t status = ESP_FAIL;
    char * error_string = "none";
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // 2. check the validity of the URL
    if ((strncmp(structNodeReceived->cptrString, "https://", 8) == 0) || (strncmp(structNodeReceived->cptrString, "http://", 7) == 0))
    {
        ESP_LOGW(TAG, "Valid URL provided");
    }
    else
    {
        error_string = "Invalid URL provided, skipping parsing";
        goto EXIT;
    }

    //3. notify user of OTA pre-preparation
    LED_assign_task(CNGW_LED_CMD_FW_UPDATE_PRE_PREPARATION, CNGW_LED_COMM);  

    // 4. download the FW from the URL
    status = SIM7080_download_file_to_SIM_from_URL(structNodeReceived->cptrString);
    if(status != ESP_OK)
    {
        error_string = "Failed to download binary file to SIM from the given URL";
        goto EXIT;
    }

    // 5. change the state of the LED
    LED_assign_task(CNGW_LED_CMD_FW_UPDATE, CNGW_LED_COMM);

    // 6. clear the required memory size
    status = update_ota_data_length_to_be_expected(SIM7080_OTA_data_recieved_length);
    if(status != ESP_OK)
    {
        error_string = "Failed to erase the ESP32 memory";
        goto EXIT;
    }

    // 7. copy the data from the SIM to the recently cleared memory in ESP32 (this includes the function process_ota_data)
    status = SIM7080_copy_bytes_from_SIM_to_ESP();
    if(status != ESP_OK)
    {
        error_string = "Failed to copy bytes from SIM7080 to ESP32";
        goto EXIT;
    }

    // 8. change the state of the LED
    LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
    
    // 9. extract information from the URL regarding the CENSE target
    char *start = strstr(structNodeReceived->cptrString, "cense_");
    char *end = strchr(start, 'b');
    if (start != NULL && end != NULL)
    {
        char extractedString[50];
        strncpy(extractedString, start, end - start);
        extractedString[end - start] = '\0';
        NodeStruct_t *structNodeReceived_internal = malloc(sizeof(NodeStruct_t));
        structNodeReceived_internal->dValue = SIM7080_OTA_data_recieved_length;
        structNodeReceived_internal->cptrString = extractedString;
        if(GW_Process_OTA_Command_Begin(structNodeReceived_internal))
        {
            ESP_LOGW(TAG, "SIM7080 begining the FW update task");
            if(SIM7080_do_Mainboard_OTA())
            {
                ESP_LOGW(TAG, "FW update done!");
                LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
                Send_GW_message_to_AWS(64, 0, "End OTA");
            }
            else
            {
                ESP_LOGE(TAG, "FW Update ERROR");
                LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_CN);
                LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_CN, LED_CHANGE_EXTENDED_DURATION);
                Send_GW_message_to_AWS(65, 0, "End OTA");
            }
        }
        free(structNodeReceived_internal);
    }
    else
    {
        error_string = "Could not decode information from the url string";
        status = ESP_FAIL;
    }

EXIT:

    // delete the temporary file from SIM7080 memory
    SIM7080_delete_file(3, SIM_OTA_FILE_NAME);

    if (status == ESP_FAIL)
    {
        ESP_LOGE(TAG, "%s", error_string);
        Send_GW_message_to_AWS(65, 0, error_string);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
        LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_COMM, LED_CHANGE_EXTENDED_DURATION);
    }
    else
    {
        LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
    }
}

/**
 * @brief Gets the command from AWS for an OTA event through SIM7080
 * Finds the target, if it is ESP32 related, or CENCE related
 * TODO: For now, built the CENCE OTA. ESP32 OTA needs to be done
 * @param structNodeReceived[in] JSON data from AWS
 * @return true if the message has a valid byte size
 */
uint8_t SIM7080_UpdateNodeFW(NodeStruct_t *structNodeReceived)
{
    ESP_LOGI(TAG, "SIM7080_UpdateNodeFW Function Called");
    if (strlen(structNodeReceived->cptrString) <= 5)
        return 0;

    bool addVerified = true;
    // GW required changes: filter out the ESP firmware from STM32 firmware
    char *strcompare = strstr(structNodeReceived->cptrString, "cense");
    if (strcompare != NULL)
    {
        ESP_LOGW(TAG, "Firmware downloaded is cense related");
        SIM7080_UpgradeNodeCenceFirmware(structNodeReceived);
        addVerified = true;
    }
    else
    {
        ESP_LOGW(TAG, "Firmware downloaded is ESP_MCU related. This is not built yet. need to build");
        //RootUtilities_UpgradeNodeFirmware(structRootWrite);
        addVerified = false;
    }


    if (addVerified)
        return 1;
    else
        return 0;
}


/**
 * @brief downloads a file from the given URL to the internal memory of the SIM7080. does the following steps in brief:
 * send command to SIM7080 to download the file to the required directory of the module
 * check for proper status in downloading the file
 * check for proper file size after download is completed
 * @param url[in] URL to download the fW from
 * @return the status of the operation
 */
esp_err_t SIM7080_download_file_to_SIM_from_URL(char *url)
{
    ESP_LOGI(TAG, "Attempting connection to URL: %s", url);
    esp_err_t status                    = ESP_FAIL;
    int status_code                     = 0;
    SIM7080_OTA_data_recieved_length    = 0;        

    // 1. create the dynamic memory to hold the AT command
    char *command = (char *)malloc(strlen(ATcmdC_HTTPTOFS) + strlen("=\"") + strlen(url) + strlen("\",\"/customer/") + strlen(SIM_OTA_FILE_NAME) + strlen("\"")+ 1);
    if (command == NULL) 
    {
        ESP_LOGE(TAG, "dynamic memory allocation error");
        return status;
    }

    // 2. Use sprintf to format cmd with required variables and send the command. after, free the resources
    sprintf(command, "%s=\"%s\",\"/customer/%s\"", ATcmdC_HTTPTOFS, url, SIM_OTA_FILE_NAME);
    status = send_msg_receive_polling((uint8_t *)command, strlen(command), 2, 90000);
    free(command);

    // 3. check if a feedback is recieved from the SIM7080
    if(status == ESP_OK)
    {
        // got a feedback on URL to FS command. now check whether the data is OK
        if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "+HTTPTOFS:") == ESP_OK)
        {
            // got details of a successful command execution. get the information to variables
            // get the location of the starting byte position and the count
            const char *target          = "+HTTPTOFS: ";
            char *pointer_to_data       = strstr((char *)recieved_data_from_SIM7080, target);

            // get the values for the status code and data length
            if(pointer_to_data != NULL)
            {
                if (sscanf(pointer_to_data + strlen(target), "%d,%d", &status_code, &SIM7080_OTA_data_recieved_length) == 2)
                {
                    // check for validity of the data recieved
                    if(status_code != HTTP_STATUS_CODE_OK)
                    {
                        ESP_LOGE(TAG, "HTTP status code is not OK");
                        status = ESP_FAIL;
                        goto EXIT;
                    }

                    if(SIM7080_OTA_data_recieved_length < SIM_OTA_MINIMUM_FILE_SIZE)
                    {
                        ESP_LOGE(TAG, "Recieved data size is too low for it to be a binary file");
                        status = ESP_FAIL;
                        goto EXIT;
                    }
                    
                    ESP_LOGW(TAG, "status_code: %d, SIM7080_OTA_data_recieved_length: %d", status_code, SIM7080_OTA_data_recieved_length);
                    // all checks upto now is OK. Now executing step 5
                }
                else
                {
                    ESP_LOGE(TAG, "Improper data recieved from SIM7080");
                    status = ESP_FAIL;
                    goto EXIT;
                }

            }
            else
            {
                ESP_LOGE(TAG, "Improper data recieved from SIM7080");
                status = ESP_FAIL;
                goto EXIT;
            }
        }
        else
        {
            // the feedback data is not of a successful command execution
            ESP_LOGE(TAG, "Improper data recieved from SIM7080");
            status = ESP_FAIL;
            goto EXIT;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error in downloading the file");
        goto EXIT;
    }

    // 4. proper data recieved. Now verify the size of the file
     if(SIM7080_get_file_size(3, SIM_OTA_FILE_NAME) != SIM7080_OTA_data_recieved_length)
     {
        ESP_LOGE(TAG, "File size mismatch");
        status = ESP_FAIL;
        goto EXIT;
     }

EXIT:

    return status;
}

/**
 * @brief deletes a file in a directory in SIM7080 memory
 * @param directory[in] The directory where the target file exists
 * @param file_name[in] The name of the file to be deleted
 * @return the status of the operation
 */
esp_err_t SIM7080_delete_file(uint8_t directory, char* file_name)
{
    esp_err_t status = ESP_FAIL;
    // 1. open FS
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSINIT, strlen(ATcmdC_CFSINIT), 1, 1000);
    if (status != ESP_OK)
    {
        goto EXIT;
    }

    // 2. create the delete file command
    char *command_delete_file = (char *)malloc(strlen("AT+CFSDFILE=3,\"") + strlen(file_name) + strlen("\"") + 1);
    if (command_delete_file == NULL) 
    {
        ESP_LOGE(TAG, "dynamic memory allocation error");
        goto EXIT;
    }

    // 3. Use sprintf to format cmd with binary_file_name
    sprintf(command_delete_file, "AT+CFSDFILE=%d,\"%s\"", directory, file_name);
    status = send_msg_receive_polling((uint8_t *)command_delete_file, strlen(command_delete_file), 1, 1000);
    free(command_delete_file);
    if (status != ESP_OK)
    {
        goto EXIT;
    }

    // 4. close FS
    send_msg_receive_polling((uint8_t *)ATcmdC_CFSTERM, strlen(ATcmdC_CFSTERM), 1, 1000);

EXIT:
    return status;
}

/**
 * @brief gets the size of a file in a directory in SIM7080 memory
 * @param directory[in] The directory where the target file exists
 * @param file_name[in] The name of the file
 * @return if the file exists, the file size of the target. else, 0
 */
uint32_t SIM7080_get_file_size(uint8_t directory, char* file_name)
{
    uint32_t file_size = 0;
    esp_err_t status   = ESP_FAIL;

    // 1. open FS
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSINIT, strlen(ATcmdC_CFSINIT), 1, 1000);
    if (status != ESP_OK)
    {
        goto EXIT;
    }
    // 2. get file size
    char *command_get_file_size = (char *)malloc(strlen("AT+CFSGFIS=3,\"") + strlen(file_name) + strlen("\"") + 1);
    if (command_get_file_size == NULL) 
    {
        ESP_LOGE(TAG, "dynamic memory allocation error");
        goto EXIT;
    }
    // 3. Use sprintf to format cmd with binary_file_name
    sprintf(command_get_file_size, "AT+CFSGFIS=%d,\"%s\"", directory, file_name);
    status = send_msg_receive_polling((uint8_t *)command_get_file_size, strlen(command_get_file_size), 1, 1000);
    free(command_get_file_size);
    if (status != ESP_OK)
    {
        goto EXIT;
    }

    // 4. extract the data from the response
    if (sscanf((char *)recieved_data_from_SIM7080, "%*[^:]: %u", &file_size) != 1)
    {
        ESP_LOGE(TAG, "Command error");
        file_size = 0;
        goto EXIT;
    }

    // 5. close FS
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSTERM, strlen(ATcmdC_CFSTERM), 1, 1000);
    if (status != ESP_OK)
    {
        file_size = 0;
        goto EXIT;
    }

EXIT:
    return file_size;
}

/**
 * @brief gets the contents of a file in bytes
 * @param file_name[in] The name of the file
 * @param read_size[in] The amount of bytes to be read
 * @param read_position[in] The position of the file from which the data begins to be read
 * @return if successful, a pointer to the read bytes. else, NULL
 */
uint8_t* SIM7080_get_bytes_from_file(char* file_name, uint16_t read_size, uint32_t read_position)
{
    // 1. create the AT command to fetch the data and send the command to SIM7080
    size_t command_length = strlen("AT+CFSRFILE=3,\"") + strlen(file_name) + strlen("\",1,,") + 10 + 10; // Assuming maximum 10 digits for read_size and read_position
    char *command_read_bytes = (char *)malloc(command_length);
    if (command_read_bytes == NULL) 
    {
        ESP_LOGE(TAG, "Dynamic memory allocation error");
        return NULL;
    }
    sprintf(command_read_bytes, "AT+CFSRFILE=3,\"%s\",1,%d,%d", file_name, read_size, read_position);
    esp_err_t status = send_msg_receive_polling((uint8_t *)command_read_bytes, strlen(command_read_bytes), 1, 1000);
    free(command_read_bytes);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Command error");
        return NULL;
    }
    
    // 2. we have recieved data. Now try reading it. get the location of the starting byte position and the count
    const char *target          = "+CFSRFILE: ";
    char *pointer_to_data       = strstr((char *)recieved_data_from_SIM7080, target);
    int amount_of_bytes_read    = 0;
    int index                   = -1;
    int index_shift             = 0;

    if (pointer_to_data != NULL)
    {
        index = pointer_to_data - (char *)recieved_data_from_SIM7080 + strlen(target);
        if (sscanf(pointer_to_data + strlen(target), "%d", &amount_of_bytes_read) == 1)
        {
            if (amount_of_bytes_read >= 0 && amount_of_bytes_read < 10)
            {
                index_shift = 4;
            }
            else if (amount_of_bytes_read >= 10 && amount_of_bytes_read < 100)
            {
                index_shift = 5;
            }
            else if (amount_of_bytes_read >= 100 && amount_of_bytes_read < UART_BUF_SIZE)
            {
                index_shift = 6;
            }

            index += index_shift;
        }
        else
        {
            ESP_LOGE(TAG, "Could not get the length of bytes read");
            return NULL;
        }
    }
    if (amount_of_bytes_read != read_size)
    {
        ESP_LOGE(TAG, "Mismatch in the amount of bytes read");
        return NULL;
    }

    // 3. correct amount of bytes recieved. proceed with the logic. allocate memory for an array of uint8_t
    uint8_t * array = (uint8_t*)malloc(read_size * sizeof(uint8_t));
    if (array == NULL)
    {
        ESP_LOGE(TAG, "Dynamic memory allocation error");
        return NULL;
    }

    // 4. copy the required data from SIM module
    for (int i = index; i < index + amount_of_bytes_read; i++)
    {
        array[i - index] = recieved_data_from_SIM7080[i];
    }

    return array;
}


/**
 * @brief copies information from the file named SIM_OTA_FILE_NAME to the ESP32 memory.
 * before this function is called, the AOT process should be initialized and global variables such as SIM7080_OTA_data_recieved_length must be properly filled
 * @return ESP_OK if successful, else ESP_FAIL
 */
esp_err_t SIM7080_copy_bytes_from_SIM_to_ESP()
{
    // 0. check integrity of variables
    if(UART_BUF_SIZE < (UART_FS_OFFSET + 100))
    {
        // the UART BUF size is not enough to handle file transfers
        ESP_LOGE(TAG, "UART_BUF_SIZE insufficient");
        goto EXIT;
    }

    esp_err_t status                = ESP_FAIL;
    uint16_t packet_size            = UART_BUF_SIZE - UART_FS_OFFSET;
    uint32_t current_file_pos       = 0;

    // 2. open FS
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSINIT, strlen(ATcmdC_CFSINIT), 1, 1000);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Command error at :%s", ATcmdC_CFSINIT);
        goto EXIT;
    }

    // 2. stop all debug messages from the library
    set_log_status(false, false, false);

    // 5. read file chunk by chunk
    while (current_file_pos < SIM7080_OTA_data_recieved_length)
    {
        // do the preliminary checks before reading chunk
        if (status == ESP_FAIL)
        {
            goto EXIT;
        }

        // read chunk
        uint8_t *binary_chunk = SIM7080_get_bytes_from_file(SIM_OTA_FILE_NAME, packet_size, current_file_pos);
        if (binary_chunk != NULL)
        {
            // copy the recieved bytes to ESP32 memory
            process_ota_data(binary_chunk, packet_size);

           // increment loop variables
            current_file_pos += packet_size;

            if(current_file_pos + packet_size > SIM7080_OTA_data_recieved_length)
            {
                // this happens in the last chunk of bytes
                packet_size = SIM7080_OTA_data_recieved_length - current_file_pos;
            }

            // free resources
            free(binary_chunk);
        }
        else
        {
            ESP_LOGE(TAG, "Error in getting bytes from SIM7080.");
            status = ESP_FAIL;
        }

    }

    // 6. resume all debug messages from the library
    set_log_status(Print_Info, Print_Info, Print_Info);

    // 7. close FS
    status = send_msg_receive_polling((uint8_t *)ATcmdC_CFSTERM, strlen(ATcmdC_CFSTERM), 1, 1000);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Command error at :%s", ATcmdC_CFSTERM);
        goto EXIT;
    }

EXIT:
    set_log_status(Print_Info, Print_Info, Print_Info); 
    return status;
}

#endif

/**
 * @brief The intermediate function which is used to check for patterns in the incoming messages from SIM7080
 *  If the recieving message has a recognized pattern, then appropriate steps are taken accordingly
 * @return ESP_OK if there is no pattern found. ESP_FAIL if there is a pattern found
 * @note by setting temp_block_SIM7080_msg_preprocessor to true, you can skip any pattern checking for one run
 */
esp_err_t SIM7080_msg_preprocessor(uint8_t *recvbuf)
{
    esp_err_t result = ESP_OK;

    // 1. check if command is set to ignore the pre_processor
    if (temp_block_SIM7080_msg_preprocessor)
    {
        temp_block_SIM7080_msg_preprocessor = false;
        return result;
    }

    // 2. check 01: "+SMSUB:" (incoming MQTT message)
    if (SIM7080_Strcmp_Data(recvbuf, GW_SIM7080.config.uart_buffer_size, "+SMSUB:") == ESP_OK)
    {
        // check for the position in the incoming string which has the "{" character
        substring_start = strstr((char *)recvbuf, "{");

        if (substring_start != NULL)
        {
            // check for the position where the JSON string ends (beginning from the "{")
            size_t substring_length     = strlen(substring_start);
            substring_end   = substring_start + substring_length - 1;

            while (substring_end > substring_start && *substring_end != '}')
            {
                substring_end--;
            }

            // the beginning and ending positions of the JSON string is found
            if (*substring_end == '}')
            {
                size_t new_length           = substring_end - substring_start + 1;
                char *truncated_substring   = (char *)pvPortMalloc((new_length + 1) * sizeof(char));

                if (truncated_substring != NULL)
                {
                    strncpy(truncated_substring, substring_start, new_length);
                    truncated_substring[new_length] = '\0';
#ifdef IPNODE
                    // if NODE is available, add the incoming command to it for it to be executed
                    if (xQueueSendToBack(nodeReadQueue, &truncated_substring, (TickType_t)0) != pdPASS)
                    {
                        ESP_LOGE(TAG, "Queue is full");
                        vPortFree(truncated_substring);
                    }
#else
                    // node is not available. so the commands are handled by the secondary utilities
                    if (xQueueSendToBack(SIM7080_AWS_Rx_queue, &truncated_substring, (TickType_t)0) != pdPASS)
                    {
                        ESP_LOGE(TAG, "Queue is full");
                        vPortFree(truncated_substring);
                    }
#endif
                }
                else
                {
                    ESP_LOGE(TAG, "Error allocating memory");
                }
            }
            else
            {
                ESP_LOGE(TAG, "Did not recieve a proper AWS command");
            }
        }

        LED_change_task_momentarily(CNGW_LED_CMD_BUSY, CNGW_LED_COMM, LED_CHANGE_MOMENTARY_DURATION);
        result = ESP_FAIL;
    }

    // 3. check 02: "+SMSTATE: 0" (disconnected from MQTT)
    else if(SIM7080_Strcmp_Data(recvbuf, GW_SIM7080.config.uart_buffer_size, "+SMSTATE: 0") == ESP_OK)
    {
        ESP_LOGE(TAG, "Disconnected from the MQTT");
        LED_assign_task(CNGW_LED_CMD_ERROR, CNGW_LED_COMM);
        set_internet_status(false);
        SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(500);
        result = ESP_FAIL;

    } 

    // 4. check 03: "+APP PDP: 0,DEACTIVE" (internet connection lost)
    else if(SIM7080_Strcmp_Data(recvbuf, GW_SIM7080.config.uart_buffer_size, "+APP PDP: 0,DEACTIVE") == ESP_OK) 
    {
        ESP_LOGE(TAG, "Internet connection lost");
        LED_assign_task(CNGW_LED_CMD_ERROR, CNGW_LED_COMM);
        set_internet_status(false);
        SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(500);
        result = ESP_FAIL;

    } 

    return result;
}


/**
 * @brief The intermediate function which is used to check for patterns in the incoming messages from SIM7080
 *  If the recieving message has a recognized pattern, then appropriate steps are taken accordingly
 * @return ESP_OK if there is no pattern found. ESP_FAIL if there is a pattern found
 * @note by setting temp_block_SIM7080_msg_preprocessor to true, you can skip any pattern checking for one run
 */
esp_err_t SIM7080_send_AWS_message(char *msg, bool success)
{
    //char message[AWS_Tx_BUFFER_SIZE] = {0};
    
    //strncpy(message, msg, message_size);
    GW_SIM7080 = *SIM_Get_Context();

    // 1. check if SIM7080 is intialized
    if (!GW_SIM7080.intialized)
    {
        ESP_LOGE(TAG, "SIM not initialized");
        return ESP_FAIL;
    }
    // 2. check if SIM7080 has internet connection
    if (!GW_SIM7080.connected_to_internet)
    {
        ESP_LOGE(TAG, "SIM no internet connection");
        return ESP_FAIL;
    }

    size_t message_size = strnlen(msg, AWS_Tx_BUFFER_SIZE - 1);
    char *command = (char *)malloc(50 * sizeof(char));
    char * ATpubC_CTRL = ATpubC_CTRL_FAIL;
    if(success)
    {
        ATpubC_CTRL = ATpubC_CTRL_SUCCESS;
    }

    if (command != NULL)
    {
        sprintf(command, "AT+SMPUB=\"%s\",%d,%d,%d", (uint8_t *)ATpubC_CTRL, message_size, AWS_PUB_QOS, AWS_PUB_RETAIN);
        if (send_msg_receive_polling((uint8_t *)command, (16 + strlen(ATpubC_CTRL) + snprintf(NULL, 0, "%d", message_size)), 1, 1000) == ESP_OK)
        {
            free(command);
            if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, ">") == ESP_OK)
            {
                if (send_msg_receive_polling((uint8_t *)msg, message_size, 2, 6000) == ESP_OK)
                {
                    if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                    {
                        return ESP_OK;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Message sending failed");
                        return ESP_FAIL;
                    }
                }
                else
                {
                    ESP_LOGE(TAG, "SIM failed to respond");
                    return ESP_FAIL;
                }
            }
        }
        else
        {
            free(command);
            ESP_LOGE(TAG, "SIM failed to respond");
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Error allocating dynamic memory");
    }

    return ESP_FAIL;
}


/**
 * @brief  this is just a placeholder for now. This function must be developed to send the LWT of the dying device to AWS
 *  for now, it just pings google one time as a POC
 */
esp_err_t ping_google()
{
    return send_msg_receive_polling((uint8_t *)ATcmdC_Google_SPING4, strlen(ATcmdC_Google_SPING4), 2, 6000);
}


/**
 * @brief  handles getting the SIM connected to the AWS. sends pre-defined commands to the SIM module sequentially.
 *  the task exits either after a successful AWS MQTT connectivity, or after a failure.
 * the commands sent in order are as follows:
 *
 * AWS_PING_SERVER              : Ping the AWS server to check proper internet connectivity and proper server availability
 * AWS_CHECK_MQTT_STATUS        : Check if the SIM is already connected to AWS
 * AWS_CONFIG_URL               : Update the URL of the AWS server to be connected to
 * AWS_CONFIG_CLIENT_ID         : Update the client ID, which is the MAC address of the ESP32 device
 * AWS_CONFIG_KEEPTIME          : Update the keep time
 * AWS_CONFIG_CLEAN_SESSION     : Update the option of clean session (set to 1)
 * AWS_CONFIG_QOS               : Update the option of Quality of service (set to 1)
 * AWS_CONFIG_RETAIN            : Update the option of retain the session (set to 1)
 * AWS_SSL_VERSION              : Update the SSL version to type 3
 * AWS_SSL_IGNORERTC            : Set the RTC to be ignored
 * AWS_SSL_CONVERT2             : SSL conversion type 2
 * AWS_SSL_CONVERT1             : SSL conversion type 1 
 * AWS_SMSSL                    : Set the certificates which will be used in the attempting connection
 * AWS_CONNECT                  : Connect to MQTT server
 * AWS_SUBSCRIBE                : Subscribe to MQTT topics
 * 
 * @return AWS_STATUS_SUCCESS if all is ok, else the corresponding error type

 */
SIM7080_AWS_status get_SIM_connected_to_AWS()
{
    SIM7080_AWS_status status = AWS_STATUS_GENERIC_ERROR;
    GW_SIM7080 = *SIM_Get_Context();
    // 1. check if SIM7080 is intialized
    if (!GW_SIM7080.intialized)
    {
        ESP_LOGE(TAG, "SIM not initialized");
        status = AWS_STATUS_SIM_INIT_ERROR;
        return status;
    }

    // 2. check if SIM7080 has internet connection
    if (!GW_SIM7080.connected_to_internet)
    {
        ESP_LOGE(TAG, "SIM no internet connection");
        status = AWS_STATUS_SERVER_CONNECTION_ERROR;
        return status;
    }

    // 3. begin steps in connecting to AWS
    SIM7080_AWS session_instance = AWS_PING_SERVER;
    bool exit_loop = false;
    while (!exit_loop)
    {
        switch (session_instance)
        {
        // 3.1 ping to the server and check proper connectivity
        case AWS_PING_SERVER:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_AWS_SPING4, strlen(ATcmdC_AWS_SPING4), 2, 6000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_CHECK_MQTT_STATUS;
                }
                else
                {
                    status = AWS_STATUS_SERVER_CONNECTION_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CHECK_MQTT_STATUS:
        {
            temp_block_SIM7080_msg_preprocessor = true;

            if (send_msg_receive_polling((uint8_t *)ATcmdQ_SMSTATE, strlen(ATcmdQ_SMSTATE), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "+SMSTATE: 0") == ESP_OK)
                {
                    session_instance = AWS_CONFIG_URL;
                }
                else
                {
                    status = AWS_STATUS_MQTT_ALREADY_CONNECTED_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CONFIG_URL:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_SMCONF_URL, strlen(ATcmdC_SMCONF_URL), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_CONFIG_CLIENT_ID;
                }
                else
                {
                    status = AWS_STATUS_CONFIG_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CONFIG_CLIENT_ID:
        {
            uint8_t MAC[6];
            if (esp_read_mac(MAC, ESP_MAC_WIFI_STA) == ESP_OK)
            {
                char command[38];
                sprintf(command, "AT+SMCONF=\"CLIENTID\",%02x:%02x:%02x:%02x:%02x:%02x", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
                if (send_msg_receive_polling((uint8_t *)command, sizeof(command), 1, 1000) == ESP_OK)
                {
                    if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                    {
                        session_instance = AWS_CONFIG_KEEPTIME;
                    }
                    else
                    {
                        status = AWS_STATUS_CONFIG_ERROR;
                        session_instance = AWS_SESSION_FAILURE;
                    }
                }
                else
                {
                    status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_ESP_MAC_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CONFIG_KEEPTIME:
        {
            char command[24];
            sprintf(command, "AT+SMCONF=\"KEEPTIME\",%d", AWS_KEEPTIME);
            if (send_msg_receive_polling((uint8_t *)command, sizeof(command), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_CONFIG_CLEAN_SESSION;
                }
                else
                {
                    status = AWS_STATUS_CONFIG_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }

        break;

        case AWS_CONFIG_CLEAN_SESSION:
        {
            char command[21];
            sprintf(command, "AT+SMCONF=\"CLEANSS\",%d", AWS_CLEAN_SESSION);
            if (send_msg_receive_polling((uint8_t *)command, sizeof(command), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_CONFIG_QOS;
                }
                else
                {
                    status = AWS_STATUS_CONFIG_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CONFIG_QOS:
        {
            char command[17];
            sprintf(command, "AT+SMCONF=\"QOS\",%d", AWS_QOS);
            if (send_msg_receive_polling((uint8_t *)command, sizeof(command), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_CONFIG_RETAIN;
                }
                else
                {
                    status = AWS_STATUS_CONFIG_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CONFIG_RETAIN:
        {
            char command[20];
            sprintf(command, "AT+SMCONF=\"RETAIN\",%d", AWS_RETAIN);
            if (send_msg_receive_polling((uint8_t *)command, sizeof(command), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_SSL_VERSION;
                }
                else
                {
                    status = AWS_STATUS_CONFIG_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_SSL_VERSION:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CSSLCFG_SSLV, strlen(ATcmdC_CSSLCFG_SSLV), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_SSL_IGNORERTC;
                }
                else
                {
                    status = AWS_STATUS_SSL_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_SSL_IGNORERTC:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CSSLCFG_RTC, strlen(ATcmdC_CSSLCFG_RTC), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_SSL_CONVERT2;
                }
                else
                {
                    status = AWS_STATUS_SSL_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;
        case AWS_SSL_CONVERT2:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CSSLCFG_CONVERT2, strlen(ATcmdC_CSSLCFG_CONVERT2), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_SSL_CONVERT1;
                }
                else
                {
                    status = AWS_STATUS_SSL_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_SSL_CONVERT1:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CSSLCFG_CONVERT1, strlen(ATcmdC_CSSLCFG_CONVERT1), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_SMSSL;
                }
                else
                {
                    status = AWS_STATUS_SSL_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_SMSSL:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_SMSSL, strlen(ATcmdC_SMSSL), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_CONNECT;
                }
                else
                {
                    status = AWS_STATUS_SSL_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_CONNECT:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_SMCONN, strlen(ATcmdC_SMCONN), 2, 60000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    session_instance = AWS_SUBSCRIBE;
                }
                else
                {
                    status = AWS_STATUS_SERVER_CONNECTION_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        case AWS_SUBSCRIBE:
        {
            uint8_t MAC[6];
            if (esp_read_mac(MAC, ESP_MAC_WIFI_STA) == ESP_OK)
            {
                char command[32];
                sprintf(command, "AT+SMSUB=\"%02x:%02x:%02x:%02x:%02x:%02x/+\",%d", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5], AWS_SUB_QOS);
                if (send_msg_receive_polling((uint8_t *)command, sizeof(command), 1, 1000) == ESP_OK)
                {
                    if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                    {
                        status = AWS_STATUS_SUCCESS;
                        session_instance = AWS_SESSION_SUCCESS;
                    }
                    else
                    {
                        status = AWS_STATUS_SUBSCRIPTION_ERROR;
                        session_instance = AWS_SESSION_FAILURE;
                    }
                }
                else
                {
                    status = AWS_STATUS_SIM_UNRESPONSIVE_ERROR;
                    session_instance = AWS_SESSION_FAILURE;
                }
            }
            else
            {
                status = AWS_STATUS_ESP_MAC_ERROR;
                session_instance = AWS_SESSION_FAILURE;
            }
        }
        break;

        default:
        {
            exit_loop = true;
        }
        break;
        }
            vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    return status;
}

/**
 * @brief  handles getting the SIM connected to the internet and then AWS. sends pre-defined commands to the SIM module sequentially.
 *  the task exits either after a successful internet connectivity, or after a failure following repeated attempts.
 * the commands sent in order are as follows:
 * AT
 * AT+CPIN?     :   Check SIM card status
 * AT+CSQ       :   Check RF signal (rssi, ber bit error rate)
 * AT+CGATT?    :   Check PS service. 1 indicates PS has attached.
 * AT+CGATT=1   :   Attach or detach from GPRS service.
 * AT+COPS?     :   Query network info, operator and network mode 9, NB-IOT network
 * AT+CGNAPN    :   Query CAT-M or NB-IOT network after the successful registration of APN
 * AT+CNACT=0,1 :   open wireless connection param 0 is PDP index, param 1 means active
 * AT+CNACT?    :   Get local IP
 * AT+SNPDPID=0 :   Select PDP index for PING as 0
 * AT+PING      :   Ping google to check proper internet connectivity
 * finally, trigger function which connects to AWS
 * if the response is a fail at any point in the sequence, the command sequence begins again at AT check.
 * if there are successive restarts, the task exits with fail after 5 successive failures
 */
void get_SIM_connected_to_internet(void *pvParameters)
{
    SIM7080_INTERNET state = SESSION_RESET;
    uint8_t failed_attempts = 0;
    uint16_t delay_between_commands = 2000;
    // start by reseting the SIM module
    LED_assign_task(CNGW_LED_CMD_CONN_PENDING, CNGW_LED_COMM);
    Reset_SIM7080();
    vTaskDelay(delay_between_commands / portTICK_PERIOD_MS);

    while (1)
    {
        // check if the amount of failures exceed the allowed amount
        if (failed_attempts > MAX_COMMUNICATION_FAILED_ATTEMPTS)
        {
            // failed attemts exceeded the allowed amount
            state = SESSION_FAILURE;
        }

        // switch to the proper step in connecting to internet
        switch (state)
        {
        case SESSION_RESET:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_AT, strlen(ATcmdC_AT), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Process_Type_01_Data((uint8_t *)recieved_data_from_SIM7080) == ESP_OK)
                {
                    // proceed to checking the SIM status
                    state = CHECK_SIM_STATUS;
                    delay_between_commands = 20;
                    LED_assign_task(CNGW_LED_CMD_CONN_STAGE_01, CNGW_LED_COMM);
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                                    // every 5 failed attempts, try resetting the Modem
                if (failed_attempts % 5 == 0)
                {
                    ESP_LOGW(TAG, "Resetting SIM module");
                    Reset_SIM7080();
                    delay_between_commands = 3000;
                }
                else
                {
                    delay_between_commands = 1000;
                }
                }
            }
            else
            {
                // no response from Modem
                failed_attempts++;
                LED_change_task_momentarily(CNGW_LED_CMD_ERROR, CNGW_LED_COMM, LED_CHANGE_MOMENTARY_DURATION);
                // every 5 failed attempts, try resetting the Modem
                if (failed_attempts % 5 == 0)
                {
                    ESP_LOGW(TAG, "Resetting SIM module");
                    Reset_SIM7080();
                    delay_between_commands = 3000;
                }
                else
                {
                    delay_between_commands = 1000;
                }
            }
        }
        break;

        case CHECK_SIM_STATUS:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdQ_CPIN, strlen(ATcmdQ_CPIN), 1, 1000) == ESP_OK)
            {

                if (SIM7080_Process_Type_02_Data((uint8_t *)recieved_data_from_SIM7080) == ESP_OK)
                {
                    // proceed to checking the RF signal
                    state = CHECK_RF_SIGNAL;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case CHECK_RF_SIGNAL:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CSQ, strlen(ATcmdC_CSQ), 1, 1000) == ESP_OK)
            {

                if (SIM7080_Process_Type_02_Data((uint8_t *)recieved_data_from_SIM7080) == ESP_OK)
                {
                    // proceed to checking the PS service
                    state = CHECK_PS_SERVICE;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case CHECK_PS_SERVICE:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdQ_CGATT, strlen(ATcmdQ_CGATT), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Process_Type_02_Data((uint8_t *)recieved_data_from_SIM7080) == ESP_OK)
                {
                    // proceed to attache GPRS service
                    state = ATTACH_GPRS_SERVICE;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case ATTACH_GPRS_SERVICE:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CGATT, strlen(ATcmdC_CGATT), 1, 75000) == ESP_OK)
            {

                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    // proceed to network query info
                    state = QUERY_NETWORK_INFO;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case QUERY_NETWORK_INFO:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdQ_COPS, strlen(ATcmdQ_COPS), 1, 45000) == ESP_OK)
            {

                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    // proceed to register APN
                    //state = REGISTER_APN;
                    state = OPEN_WIRELESS_CONNECTION;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;
        //NOTE: not being executed
        case REGISTER_APN:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdQ_COPS, strlen(ATcmdQ_COPS), 1, 90000) == ESP_OK)
            {

                if (SIM7080_Process_Type_02_Data((uint8_t *)recieved_data_from_SIM7080) == ESP_OK)
                {
                    // proceed to open wireless connection
                    state = OPEN_WIRELESS_CONNECTION;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case OPEN_WIRELESS_CONNECTION:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_CNACT, strlen(ATcmdC_CNACT), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    // get list of local IP addresses
                    state = GET_LOCAL_IP;
                }
                else
                {
                    // response from Modem is ERROR, but this is because the connection is already established
                    // get list of local IP addresses
                    state = GET_LOCAL_IP;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case GET_LOCAL_IP:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdQ_CNACT, strlen(ATcmdQ_CNACT), 1, 1000) == ESP_OK)
            {
                // proceed to setting the PDP index
                state = SET_PDP_INDEX;
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case SET_PDP_INDEX:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_SNPDPID, strlen(ATcmdC_SNPDPID), 1, 1000) == ESP_OK)
            {
                if (SIM7080_Process_Type_01_Data((uint8_t *)recieved_data_from_SIM7080) == ESP_OK)
                {
                    // proceed to checking internet connection
                    state = PING_GOOGLE;
                }
                else
                { // response from Modem is ERROR
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case PING_GOOGLE:
        {
            if (send_msg_receive_polling((uint8_t *)ATcmdC_Google_SPING4, strlen(ATcmdC_Google_SPING4), 2, 6000) == ESP_OK)
            {
                if (SIM7080_Strcmp_Data((uint8_t *)recieved_data_from_SIM7080, GW_SIM7080.config.uart_buffer_size, "OK") == ESP_OK)
                {
                    // proceed to exit
                    state = SESSION_SUCCESS;
                }
                else
                {
                    // response from Modem is ERROR
                    ESP_LOGE(TAG, "ping to google failed. trying again...");
                    failed_attempts++;
                }
            }
            else
            {
                failed_attempts++;
            }
        }
        break;

        case SESSION_FAILURE:
        {
            ESP_LOGE(TAG, "SIM failed to connect to internet");
            LED_assign_task(CNGW_LED_CMD_ERROR,CNGW_LED_COMM);
            set_internet_status(false);
            SIM7080_internet_handler = NULL;
            xTimerDelete(SIM7080_internet_reconnection_handler, 0);
            SIM7080_internet_reconnection_handler = NULL;
            //re attempt internet connection
            SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(20000);
            vTaskDelete(NULL);
        }
        break;

        case SESSION_SUCCESS:
        {
            ESP_LOGI(TAG, "SIM connected to internet");
            LED_assign_task(CNGW_LED_CMD_CONN_STAGE_02, CNGW_LED_COMM);
            set_internet_status(true);
            Send_GW_message_to_AWS(64, 0, "SIM7080 connected to internet");
            SIM7080_AWS_status AWS_result = get_SIM_connected_to_AWS();

            if (AWS_result == AWS_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "SIM connected to AWS");
                set_AWS_status(true);
                LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
                //notify AWS of GW power ON
                Send_GW_message_to_AWS(64, 0, "SIM7080 connected to AWS");

            }
            else if (AWS_result == AWS_STATUS_MQTT_ALREADY_CONNECTED_ERROR)
            {
                ESP_LOGI(TAG, "SIM already connected to AWS");
                set_AWS_status(true);
                LED_assign_task(CNGW_LED_CMD_IDLE, CNGW_LED_COMM);
                //notify AWS of GW power ON
                Send_GW_message_to_AWS(64, 0, "SIM7080 connected to AWS");

            }
            else
            {
                ESP_LOGE(TAG, "SIM failed to connect to AWS");
                set_AWS_status(false);
                LED_assign_task(CNGW_LED_CMD_ERROR, CNGW_LED_COMM);
                SIM7080_internet_handler = NULL;
                xTimerDelete(SIM7080_internet_reconnection_handler, 0);
                SIM7080_internet_reconnection_handler = NULL;
                 //re attempt internet connection
                SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(20000);
                Send_GW_message_to_AWS(65, 0, "SIM7080 failed to connect to AWS");
                vTaskDelete(NULL);
           
            }
            SIM7080_internet_handler = NULL;
            xTimerDelete(SIM7080_internet_reconnection_handler, 0);
            SIM7080_internet_reconnection_handler = NULL;
            vTaskDelete(NULL);
        }
        break;

        default:
        {
            SIM7080_internet_handler = NULL;
            xTimerDelete(SIM7080_internet_reconnection_handler, 0);
            SIM7080_internet_reconnection_handler = NULL;
            vTaskDelete(NULL);
        }
        break;
        }

        vTaskDelay(delay_between_commands / portTICK_PERIOD_MS);
    }
}

#endif