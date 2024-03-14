#ifndef SIM7080_LIB_H
#define SIM7080_LIB_H

#include "string.h"
#include "driver/uart.h"
#include "esp_log.h"


typedef struct SIM7080_msg_rcv_task_params_t
{
    uint8_t total_messages;
    uint16_t timeout;
    uint8_t* recv_msg;
    uint16_t recv_msg_size;
    const char * recv_command_1;
    const char * recv_command_2;


}__attribute__((packed)) SIM7080_msg_rcv_task_params_t;

typedef struct SIM7080_config_t
{
    uart_port_t uart_port;
    uint8_t uart_tx_pin;
    uint8_t uart_rx_pin;
    uint8_t wakeup_pin;
    uint16_t uart_buffer_size;
    uint8_t queue_length; 				 

} __attribute__((packed)) SIM7080_config_t;

typedef struct SIM7080_log_t
{
    bool print_Tx_info;
    bool print_Rx_info;
    bool print_log_info;

} __attribute__((packed)) SIM7080_log_t;


typedef struct SIM7080_communication_t
{
    SemaphoreHandle_t   Semaphore_UART_Resource;
    QueueHandle_t       Recieve_queue; 
    uint8_t*            recieved_message;
    esp_err_t (*pre_process_recieved_message)(uint8_t* recvbuf);	 

} __attribute__((packed)) SIM7080_communication_t;


typedef struct SIM7080_t
{
    SIM7080_config_t config;
    SIM7080_communication_t comm;
    SIM7080_log_t log;
    bool intialized;
    bool connected_to_internet;
    bool connected_to_AWS;
    bool is_busy;		 

} __attribute__((packed)) SIM7080_t;


typedef enum SIM7080_FS_Dir
{
    CUSTAPP = 0,
    FOTA,
    DATATX,
    CUSTOMER

}SIM7080_FS_Dir;

typedef enum SIM7080_FS_Mode
{
    WRITE_DATA_FROM_BEGINNING = 0,
    APPEND_DATA_TO_END,

}SIM7080_FS_Mode;

typedef struct SIM7080_FS_t
{
    SIM7080_FS_Dir directory;
    SIM7080_FS_Mode mode;
    const char * file_name;
    const char* start_pos;
    const char* end_pos;
    uint16_t timeout;
    bool check_file_availability;



} __attribute__((packed)) SIM7080_FS_t;


esp_err_t SIM7080_setup(SIM7080_t *SIM_module);
esp_err_t SIM7080_Strcmp_Data(uint8_t *data, uint8_t data_length, const char *character);
esp_err_t SIM7080_FS_write_file(SIM7080_FS_t * File);
esp_err_t set_internet_status(bool state);
esp_err_t set_log_status(bool print_Tx_info, bool print_Rx_info, bool print_log_info);
esp_err_t set_AWS_status(bool state);
SIM7080_t *SIM_Get_Context(void);
void SIM7080_Transmit_Data(void *pvParameters);
void SIM7080_Receive_Data(void *pvParameters);
esp_err_t consume_SIM7080_message(uint8_t *message, size_t length);
esp_err_t send_msg_receive_polling(uint8_t *message, uint16_t transmit_message_size, uint8_t amount_of_commands_expected, uint32_t timeout);
esp_err_t send_msg_receive_interrupt(uint8_t *message, uint16_t transmit_message_size, uint8_t amount_of_commands_expected, uint16_t timeout, uint8_t recieve_message[], uint16_t recieve_message_size);
void send_msg_receive_task(void *pvParameters);

#endif
