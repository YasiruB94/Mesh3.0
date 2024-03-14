/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: SIM7080.h
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
#ifndef SIM7080_H
#define SIM7080_H
#include "includes/SpacrGateway_commands.h"
#include "includes/aws.h"
#include "driver/uart.h"
#include "SIM7080_lib.h"
#include "esp_system.h"

extern QueueHandle_t SIM7080_AWS_Tx_queue;
extern QueueHandle_t SIM7080_AWS_Rx_queue;

// UART configuration definitions
#define SIM7080_TX                          25
#define SIM7080_RX                          26
#define SIM7080_PULSE                       27
#define UART_BUF_SIZE                       2000  //going beyond 2000 makes the ESP panic when trying to read the entire certificate
#define UART_QUEUE_LENGTH                   2
#define SIM7080_UART_PORT                   UART_NUM_1
#define MAX_COMMUNICATION_FAILED_ATTEMPTS   10
// AWS MQTT session information
#define AWS_KEEPTIME                        60
#define AWS_CLEAN_SESSION                   1
#define AWS_QOS                             1
#define AWS_SUB_QOS                         1
#define AWS_PUB_QOS                         1
#define AWS_RETAIN                          0
#define AWS_PUB_RETAIN                      0
#define AWS_Tx_BUFFER_SIZE                  500
// AT internet commands
#define ATcmdC_AT                   "AT"
#define ATcmdQ_CPIN                 "AT+CPIN?"
#define ATcmdC_CSQ                  "AT+CSQ"
#define ATcmdQ_CGATT                "AT+CGATT?"
#define ATcmdC_CGATT                "AT+CGATT=1"
#define ATcmdQ_COPS                 "AT+COPS?"
#define ATcmdC_CNACT                "AT+CNACT=0,1"
#define ATcmdQ_CNACT                "AT+CNACT?"
#define ATcmdC_SNPDPID              "AT+SNPDPID=0"
#define ATcmdC_Google_SPING4        "AT+SNPING4=\"8.8.8.8\",1,16,5000"
// AT AWS commands
#define ATcmdC_AWS_SPING4           "AT+SNPING4=a1yr1nntapo4lc.iot.us-west-2.amazonaws.com,1,16,5000"
#define ATcmdC_SMCONF_URL           "AT+SMCONF=\"URL\", a1yr1nntapo4lc.iot.us-west-2.amazonaws.com,8883"
#define ATcmdC_CSSLCFG_SSLV         "AT+CSSLCFG=\"SSLVERSION\",0,3"
#define ATcmdC_CSSLCFG_RTC          "AT+CSSLCFG=\"IGNORERTCTIME\",0,1"
#define ATcmdC_CSSLCFG_CONVERT2     "AT+CSSLCFG=\"CONVERT\",2,rootCA.pem"
#define ATcmdC_CSSLCFG_CONVERT1     "AT+CSSLCFG=\"CONVERT\",1,deviceCert.crt,devicePKey.key"
#define ATcmdC_SMSSL                "AT+SMSSL=1,rootCA.pem,deviceCert.crt"
#define ATcmdC_SMCONN               "AT+SMCONN"
#define ATcmdC_SMDISC               "AT+SMDISC"
#define ATcmdQ_SMSTATE              "AT+SMSTATE?"
// AT FS commands
#define ATcmdC_CFSINIT              "AT+CFSINIT"
#define ATcmdQ_CFSGFRS              "AT+CFSGFRS?"
#define ATcmdC_CFSTERM              "AT+CFSTERM"
// AT AWS pubsub commands
#define ATpubC_CTRL_SUCCESS         "/controldata/success"
#define ATpubC_CTRL_FAIL            "/controldata/fail"
// AT OTA commands
#define ATcmdC_HTTPTOFS             "AT+HTTPTOFS"
// OTA definitions
#define SIM_OTA_FILE_NAME           "temporary.bin"
#define SIM_OTA_MINIMUM_FILE_SIZE   2000
#define HTTP_STATUS_CODE_OK         200
#define UART_FS_OFFSET              75



// typedef to get connected to internet
typedef enum SIM7080_INTERNET
{
    SESSION_RESET,
    CHECK_SIM_STATUS,
    CHECK_RF_SIGNAL,
    CHECK_PS_SERVICE,
    ATTACH_GPRS_SERVICE,
    QUERY_NETWORK_INFO,
    REGISTER_APN,
    OPEN_WIRELESS_CONNECTION,
    GET_LOCAL_IP,
    SET_PDP_INDEX,
    PING_GOOGLE,
    SESSION_FAILURE,
    SESSION_SUCCESS

}SIM7080_INTERNET;

// typedef to get connected to AWS
typedef enum SIM7080_AWS
{
    AWS_SESSION_SUCCESS = 0,
    AWS_PING_SERVER,
    AWS_CHECK_MQTT_STATUS,
    AWS_CONFIG_URL,
    AWS_CONFIG_CLIENT_ID,
    AWS_CONFIG_KEEPTIME,
    AWS_CONFIG_CLEAN_SESSION,
    AWS_CONFIG_QOS,
    AWS_CONFIG_RETAIN,
    AWS_SSL_VERSION,
    AWS_SSL_IGNORERTC,
    AWS_SSL_CONVERT2,
    AWS_SSL_CONVERT1,
    AWS_SMSSL,
    AWS_CONNECT,
    AWS_SUBSCRIBE,
    AWS_SESSION_FAILURE,

}SIM7080_AWS;

typedef enum SIM7080_AWS_status
{
    AWS_STATUS_SUCCESS = 0,
    AWS_STATUS_GENERIC_ERROR,
    AWS_STATUS_SIM_INIT_ERROR,
    AWS_STATUS_SERVER_CONNECTION_ERROR,
    AWS_STATUS_MQTT_ALREADY_CONNECTED_ERROR,
    AWS_STATUS_SIM_UNRESPONSIVE_ERROR,
    AWS_STATUS_CONFIG_ERROR,
    AWS_STATUS_SUBSCRIPTION_ERROR,
    AWS_STATUS_SSL_ERROR,
    AWS_STATUS_ESP_MAC_ERROR,

}SIM7080_AWS_status;

esp_err_t init_SIM7080();
esp_err_t Execute_AT_CMD(char * command, uint8_t num_commands);
esp_err_t SIM7080_Process_Type_01_Data(uint8_t *data);
esp_err_t SIM7080_Process_Type_02_Data(uint8_t *data);
esp_err_t ping_google();
esp_err_t Send_Certs_To_SIM();
esp_err_t SIM7080_Clear_Queues();
void get_SIM_connected_to_internet(void *pvParameters);
SIM7080_AWS_status get_SIM_connected_to_AWS();
esp_err_t SIM7080_msg_preprocessor(uint8_t* recvbuf);
void SIM7080_Connect_to_Internet_And_AWS(void *timer);
esp_err_t SIM7080_Retry_Connect_to_Internet_And_AWS_After_Delay(uint16_t delay);
esp_err_t SIM7080_send_AWS_message(char *msg, bool success);

// The below functions will only be needed when the GW is acting as only SIM7080 driven unit.
// (if IPNODE is defined, the OTA information must be sent via IPNODE)
#ifndef IPNODE
uint8_t SIM7080_UpdateNodeFW(NodeStruct_t *structNodeReceived);
void SIM7080_UpgradeNodeCenceFirmware(NodeStruct_t *structNodeReceived);
esp_err_t SIM7080_copy_bytes_from_SIM_to_ESP();
uint8_t* SIM7080_get_bytes_from_file(char* file_name, uint16_t read_size, uint32_t read_position);
esp_err_t SIM7080_download_file_to_SIM_from_URL(char * url);
uint32_t SIM7080_get_file_size(uint8_t directory, char* file_name);
esp_err_t SIM7080_delete_file(uint8_t directory, char* file_name);
#endif


#endif
#endif