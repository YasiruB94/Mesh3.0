#ifdef ROOT
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "includes/aws.h"
#include "includes/root_utilities.h"
#include "string.h"
#include "cJSON.h"
static const char *TAG = "SpacrAWS";
uint32_t uiSubscribeCounter = 0;
//If auto re-enable for some reason doesn't get enabled we log the message
void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
    ESP_LOGW(TAG, "AWS Disconnected");
    IoT_Error_t rc = FAILURE;

    if (NULL == pClient)
    {
        return;
    }

    ESP_LOGE(TAG, "Log to backend");
}

//Assinging setup param values
void AWSConnectionSetup(char *macAddress)
{
    //if null need to post to a different topic
    mqttInitParams.enableAutoReconnect = false;
    mqttInitParams.pHostURL = "a1yr1nntapo4lc-ats.iot.us-west-2.amazonaws.com";
    mqttInitParams.port = 8883;
    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;
    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;
    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = macAddress;
    connectParams.clientIDLen = (uint16_t)strlen(macAddress);
    connectParams.isWillMsgPresent = false;
    //Think about adding a will message here
}

void iot_subscribe_callback_handler_node(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                         IoT_Publish_Message_Params *params, void *pData)
{
    // ESP_LOGI(TAG, "Node callback");
    // ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);
    char *payload = (char *)pvPortMalloc((strlen(topicName) + 1) * sizeof(char));
    memset(payload, 0, topicNameLen);
    strcpy(payload, topicName);

    if (xQueueSendToBack(nodeCommandQueue, &payload, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(payload);
        //Report back to AWS here letting us know that the queue is full and to wait for the next command
        //The queue most likely will neveer fill up but you never know
    }
    memset((char *)params->payload, 0, (int)params->payloadLen);
    memset(topicName, 0, topicNameLen);
    ESP_LOGI(TAG, "Stack for task '%s': %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(TAG, "Subscribe Callback Counter: %d", uiSubscribeCounter);
    uiSubscribeCounter++;
}

void iot_subscribe_callback_handler_root(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                         IoT_Publish_Message_Params *params, void *pData)
{
    // ESP_LOGI(TAG, "Root callback");
    ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int)params->payloadLen, (char *)params->payload);
    char *payload = (char *)pvPortMalloc((strlen((char *)params->payload) + 1) * sizeof(char));
    memset(payload, 0, (int)params->payloadLen);
    strcpy(payload, (char *)params->payload);
    if (xQueueSendToBack(rootCommandQueue, &payload, (TickType_t)0) != pdPASS)
    {
        ESP_LOGE(TAG, "Queue is full");
        vPortFree(payload);
        //Report back to AWS here letting us know that the queue is full and to wait for the next command
        //The queue most likely will neveer fill up but you never know
    }
    memset((char *)params->payload, 0, (int)params->payloadLen);
    ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
}

void aws_subscribe_topic(const char *topic, pApplicationHandler_t handler)
{
    ESP_LOGI(TAG, "Subscribing...");
    IoT_Error_t rc = FAILURE;
    rc = aws_iot_mqtt_subscribe(&client, topic, strlen(topic), QOS0, handler, NULL);
    if (SUCCESS != rc)
    {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        ESP_LOGE(TAG, "Log to backend");
    }
}

void aws_publish_topic(char *topic, IoT_Publish_Message_Params *paramsQOS1)
{
    ESP_LOGI(TAG, "Publishing...");
    IoT_Error_t rc = FAILURE;
    rc = aws_iot_mqtt_publish(&client, topic, strlen(topic), paramsQOS1);
    if (rc == MQTT_REQUEST_TIMEOUT_ERROR)
    {
        ESP_LOGW(TAG, "QOS1 publish ack not received.");
        rc = SUCCESS;
        //maybe we convert this into a while loop or we return the RC to let the function know that it was not successful
    }
    else
    {
        // ESP_LOGW(TAG, "QOS1 publish ack received.");
    }
    ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
}

void AWS_CreateTopics()
{
    //SensorData topic
    char sensData[13] = "/sensordata/";
    sensorLogTopic = (char *)realloc(sensorLogTopic, (strlen(sensData) + strlen(orgID) + 1) * sizeof(char));
    strcpy(sensorLogTopic, orgID);
    strcat(sensorLogTopic, sensData);

    //ControlData topic
    char controlData[14] = "/controldata/";
    controlLogTopic = (char *)realloc(controlLogTopic, (strlen(controlData) + strlen(orgID) + 1) * sizeof(char));
    strcpy(controlLogTopic, orgID);
    strcat(controlLogTopic, sensData);

    //log topic
    char logs[7] = "/logs/";
    logTopic = (char *)realloc(logTopic, (strlen(logs) + strlen(deviceMACStr) + 1) * sizeof(char));
    strcpy(logTopic, deviceMACStr);
    strcat(logTopic, logs);

    char controlDataS[21] = "/controldata/success";
    controlLogTopicSuccess = (char *)realloc(controlLogTopicSuccess, (strlen(controlDataS) + strlen(orgID) + 1) * sizeof(char));
    strcpy(controlLogTopicSuccess, orgID);
    strcat(controlLogTopicSuccess, controlDataS);

    char controlDataF[18] = "/controldata/fail";
    controlLogTopicFail = (char *)realloc(controlLogTopicFail, (strlen(controlDataF) + strlen(orgID) + 1) * sizeof(char));
    strcpy(controlLogTopicFail, orgID);
    strcat(controlLogTopicFail, controlDataF);
}

void AWS_AWSTask()
{
    ESP_LOGI(TAG, "Initiating AWS parameters");
    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
    IoT_Error_t rc = FAILURE;
    mqttInitParams = iotClientInitParamsDefault;
    connectParams = iotClientConnectParamsDefault;

    //Utilities_GetDeviceMac(deviceMACStr);
    AWSConnectionSetup(deviceMACStr);

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if (SUCCESS != rc)
    {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        ESP_LOGE(TAG, "Log to backend");
    }

    ESP_LOGI(TAG, "Connecting to AWS...");
    uint8_t awsReconCounter = 0;
    do
    {
        if (awsReconCounter >= 5)
        {
            ESP_LOGE(TAG, "Log to backend and restart");
        }
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if (SUCCESS != rc)
        {
            ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        awsReconCounter++;
    } while (SUCCESS != rc);

    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if (SUCCESS != rc)
    {
        ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        ESP_LOGE(TAG, "Log to backend");
        //creating a log of errors and then sending them
    }

    char deviceTopic[20];
    strcpy(deviceTopic, deviceMACStr);
    strcat(deviceTopic, "/+");

    aws_subscribe_topic(deviceTopic, iot_subscribe_callback_handler_node);
    aws_subscribe_topic(deviceMACStr, iot_subscribe_callback_handler_root);

    IoT_Publish_Message_Params paramsQOS1;
    paramsQOS1.qos = QOS1;
    paramsQOS1.isRetained = 0;
    AWSPub_t *stuctAWSPub;
    while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc || NETWORK_RECONNECT_TIMED_OUT_ERROR == rc || NETWORK_DISCONNECTED_ERROR == rc))
    {
        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if (NETWORK_ATTEMPTING_RECONNECT == rc)
        {
            //rc = aws_iot_mqtt_yield(&client, 1000);
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }
        if (NETWORK_RECONNECT_TIMED_OUT_ERROR == rc || NETWORK_DISCONNECTED_ERROR == rc)
        {
            // either:
            // 1. maximum back-off time reached for the iot client - it has timed out, or
            // 2. auto reconnect is turned off
            // manually attempt to reconnect again
            ESP_LOGW(TAG, "Manually reconnecting");
            rc = aws_iot_mqtt_attempt_reconnect(&client);
        }
        if (AWSPublishQueue != NULL)
        {
            if (xQueueReceive(AWSPublishQueue, &stuctAWSPub, (TickType_t)0))
            {
                paramsQOS1.payload = (void *)stuctAWSPub->cptrPayload;
                paramsQOS1.payloadLen = strlen(stuctAWSPub->cptrPayload);
                ESP_LOGW(TAG, "Topic Received: %s", stuctAWSPub->cptrTopic);
                ESP_LOGW(TAG, "Payload Received: %s", stuctAWSPub->cptrPayload);
                aws_publish_topic(stuctAWSPub->cptrTopic, &paramsQOS1);
                //memset(stuctAWSPub->cptrPayload, 0, strlen(stuctAWSPub->cptrPayload));
                vPortFree(stuctAWSPub->cptrPayload);
                vPortFree(stuctAWSPub->cptrTopic);
                vPortFree(stuctAWSPub);
            }
        }
        vTaskDelay(10 / portTICK_RATE_MS); //leave this for watchdog
    }
}
#endif