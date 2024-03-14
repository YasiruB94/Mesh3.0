
#ifndef _AWS_H_
#define _AWS_H_
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "root_operations.h"

extern const uint8_t aws_root_ca_sim_pem_start[] asm("_binary_aws_root_ca_SIM_pem_start");
extern const uint8_t aws_root_ca_sim_pem_end[] asm("_binary_aws_root_ca_SIM_pem_end");
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

AWS_IoT_Client client;
IoT_Client_Init_Params mqttInitParams;
IoT_Client_Connect_Params connectParams;

extern void AWS_CreateTopics();
extern void AWS_AWSTask();
// void AWS_AWSPub();

#endif