/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: SPI_comm.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in SPI communication with CENCE mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#include "gw_includes/SPI_comm.h"
static const char *TAG = "SPI_comm";

QueueHandle_t GW_response_queue;
QueueHandle_t CN_message_queue;
bool cn_message_queue_error = false;
bool SPI_freed = false;

//Called after a transaction is queued and ready for pickup by master. Use this to set the handshake line high.
void post_setup_cb(spi_slave_transaction_t *trans)
{
    Backward_Data_Ready_Sequence();
}

//Called after transaction is sent/received. Use this to set the handshake line low.
void post_trans_cb(spi_slave_transaction_t *trans)
{
    Tx_Complete_Sequence();
}

esp_err_t init_GW_SPI_communication()
{
    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,};

    // Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 10,// original = 1
        .flags = 0,
        .post_setup_cb = post_setup_cb,
        .post_trans_cb = post_trans_cb};

    // Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_CS, GPIO_PULLUP_ONLY);

    // initialize the GW_response queue
    GW_response_queue = xQueueCreate(QUEUE_LENGTH, BUFFER * sizeof(uint8_t));
    // initialize the CN_message queue
    CN_message_queue = xQueueCreate(QUEUE_LENGTH, BUFFER * sizeof(uint8_t));
    // Initialize SPI slave interface
    esp_err_t result = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, 1);
    SPI_freed = false;
    if (result == ESP_OK)
    {
        xTaskCreate(GW_Trancieve_Data, "GW_Trancieve_Data", 4096, NULL, 5, NULL);
        xTaskCreate(GW_process_received_data, "GW_process_received_data", 4096, NULL, 5, NULL);
    }
    else
    {
        ESP_LOGE(TAG, "spi_slave_initialize failed (%s): ", esp_err_to_name(result));
    }
    return result;
}


void GW_Trancieve_Data(void *pvParameters)
{
    ESP_LOGI(TAG, "GW_Trancieve_Data");
    uint8_t data_to_send[BUFFER];
    uint8_t *recvbuf = (uint8_t *)heap_caps_malloc(BUFFER, MALLOC_CAP_DMA);
    spi_slave_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    
    while (1)
    {
        // 1. Set the receiving buffer to all zeros
        memset(recvbuf, 0, BUFFER);

        if (uxQueueMessagesWaiting(GW_response_queue) > 0)
        {
            memset(data_to_send, 0, BUFFER);
            // there are messages waiting to be sent to the CN
            if (xQueueReceive(GW_response_queue, data_to_send, 0) == pdTRUE)
            {
                // get the data to the data_to_send buffer
                trans.tx_buffer = data_to_send;

                // notify CN that there is information to be read
                Backward_Data_Sequence();
            }
            else
            { // failed to read data from the queue
                ESP_LOGE(TAG, "failed to read data from the queue");
                trans.tx_buffer = NULL;
            }
        }
        else
        {
            // there are no messages to be sent
            trans.tx_buffer = NULL;
        }

        trans.rx_buffer = recvbuf;
        trans.length = BUFFER * 8;
        //Check if the SPI Host is freed. If so, terminate the task
        if(SPI_freed)
        {
            vTaskDelete(NULL);
        }
        // Receive data from master
        esp_err_t ret = spi_slave_transmit(HSPI_HOST, &trans, 1);
        if (ret == ESP_OK)
        {
            // data from the CN
            if (is_all_zeros(recvbuf, BUFFER) != ESP_OK)
            {
                if (memcmp(data_to_send, recvbuf, sizeof(&data_to_send)) != 0)
                {
                    // the data is not the same as the sent data.
                    if (xQueueSend(CN_message_queue, recvbuf, 1) != pdTRUE)
                    {
                        ESP_LOGE(TAG, "CN_message_queue is full");
                        cn_message_queue_error = true;
                    }
                    else
                    {
                        cn_message_queue_error = false;
                    }
                }
            }
        }
    }
}

esp_err_t is_all_zeros(const uint8_t *array, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if ((int)array[i] != 0)
        {
            return ESP_FAIL; // If any element is non-zero, return false
        }
    }

    return ESP_OK; // All elements are zero
}

esp_err_t consume_GW_message(uint8_t *message)
{
    if (xQueueSend(GW_response_queue, message, 1) == pdTRUE)
    {
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "error consuming message");
        return ESP_FAIL;
    }
}

esp_err_t free_SPI(CNGW_Firmware_Binary_Type target_MCU_int)
{
    ESP_LOGI(TAG, "Freeing the SPI resources...");
    SPI_freed = true;
    esp_err_t result = spi_slave_free(HSPI_HOST);
    ESP_LOGI(TAG, "Status: %s. restarting Gateway...", esp_err_to_name(result));
    switch (target_MCU_int)
    {
    case CNGW_FIRMWARE_BINARY_TYPE_cn_mcu:
    {
        delayed_ESP_Restart(30000);
    }
    break;
    case CNGW_FIRMWARE_BINARY_TYPE_sw_mcu:
    {
        delayed_ESP_Restart(12000);
    }
    break;
    case CNGW_FIRMWARE_BINARY_TYPE_dr_mcu:
    {
        delayed_ESP_Restart(24000);
    }
    break;
    default:
    {
        delayed_ESP_Restart(24000);
    }
    break;
    }

    return result;
}

void GW_process_received_data(void *pvParameters)
{
    uint8_t receivedData[BUFFER];
    while (1)
    {
        memset(receivedData, 0, BUFFER);
        if (xQueueReceive(CN_message_queue, receivedData, portMAX_DELAY) == pdTRUE)
        {
            size_t startIndex = 0;
            while (startIndex < BUFFER)
            {
                startIndex += Parse_1_Frame_Partial(receivedData, BUFFER, startIndex);
            }
        }
    }
}

size_t Parse_1_Frame_Partial(uint8_t *packet, size_t dataSize, size_t startIndex)
{
    packet += startIndex;
    dataSize -= startIndex;
    return Parse_1_Frame(packet, dataSize);
}
#endif