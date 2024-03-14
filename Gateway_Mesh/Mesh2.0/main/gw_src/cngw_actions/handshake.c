/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: handshake.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in achieving a proper handshake with the mainboard
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(GATEWAY_ETHERNET) || defined(GATEWAY_SIM7080)
#include "gw_includes/handshake.h"
#include "gw_includes/ccp_util.h"
static const char *TAG = "handshake";

#ifdef GATEWAY_DEBUGGING
static bool print_all_frame_info    = false;
static bool print_header_frame_info = true;
static bool avoid_HMAC_functions    = false;
#else
static bool print_all_frame_info    = false;
static bool print_header_frame_info = false;
static bool avoid_HMAC_functions    = false;
#endif

void analyze_CNGW_Handshake_CN1_t(const uint8_t *recvbuf, int size)
{
    CNGW_Handshake_CN1_Frame_t frame;
    memcpy(&frame, recvbuf, sizeof(frame));
    cn_board_info.cn_config.cabinet_number = frame.message.cabinet_number;

    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "frame decoded as CNGW_Handshake_CN1_Frame_t");
        ESP_LOGI(TAG, "frame.header.command_type: %d"       , frame.header.command_type);
        ESP_LOGI(TAG, "frame.header.data_size: %u"          , frame.header.data_size);
        ESP_LOGI(TAG, "frame.header.crc: %u"                , frame.header.crc);
        ESP_LOGI(TAG, "frame.message.cabinet_number: %u"    , frame.message.cabinet_number);
        printf("frame.message.challenge: ");
        for (int i = 0; i < CNGW_CHALLENGE_RESPONSE_LENGTH; i++)
        {
            printf("%d ", frame.message.challenge[i]);
        }
        printf("\n");
        ESP_LOGI(TAG, "frame.message.command: %d"           , frame.message.command);
        printf("frame.message.hmac: ");
        for (int i = 0; i < CNGW_HMAC_LENGTH; i++)
        {
            printf("%d ", frame.message.hmac[i]);
        }
        printf("\n");
        printf("frame.message.mainboard_serial: ");
        for (int i = 0; i < CNGW_SERIAL_NUMBER_LENGTH; i++)
        {
            printf("%02X ", frame.message.mainboard_serial[i]);
        }
        printf("\n");
    }
    prepare_CN_handshake_01_feedback(&frame);
}

void analyze_CNGW_Handshake_CN2_t(const uint8_t *recvbuf, int size)
{
    CNGW_Handshake_CN2_Frame_t frame;
    memcpy(&frame, recvbuf, sizeof(frame));

    if (print_all_frame_info)
    {
        ESP_LOGI(TAG, "frame decoded as CNGW_Handshake_CN2_Frame_t");
        ESP_LOGI(TAG, "frame.header.command_type: %d"       , frame.header.command_type);
        ESP_LOGI(TAG, "frame.header.data_size: %u"          , frame.header.data_size);
        ESP_LOGI(TAG, "frame.header.crc: %u"                , frame.header.crc);
        ESP_LOGI(TAG, "frame.message.command: %u"           , frame.message.command);
        ESP_LOGI(TAG, "frame.message.status: %u"            , frame.message.status);
        printf("frame.message.hmac: ");
        for (int i = 0; i < CNGW_HMAC_LENGTH; i++)
        {
            printf("%d ", frame.message.hmac[i]);
        }
        printf("\n");
    }
    prepare_CN_handshake_02_feedback(&frame);
}

void prepare_CN_handshake_01_feedback(CNGW_Handshake_CN1_Frame_t *cn)
{
    if(print_header_frame_info)
    {
        ESP_LOGI(TAG, "prepare_CN_handshake_01_feedback");
    }
    // 1. variable initialization
    CNGW_Handshake_CN1_t        * cn1       = &cn->message;
    CNGW_Handshake_Status       status      = CNGW_Handshake_STATUS_FAILED;
    CNGW_Handshake_GW1_Frame_t  gw1         = {0};
    esp_err_t                   HMAC_Result = ESP_FAIL;
    size_t                      msg_length;

    // 2. HMAC verification for incoming handshake message
    msg_length = sizeof(*cn1) - sizeof(cn1->hmac);
    if (!avoid_HMAC_functions)
    {
        HMAC_Result = ATMEL_Validate_HMAC((const uint8_t *)cn1, msg_length, 3, cn1->hmac);

        // 3. if the incoming HMAC is validated, create the challenge MAC for mainboard to verify
        if (HMAC_Result == ESP_OK)
        {
            ATMEL_Challenge_MAC(cn1->challenge, 3, gw1.message.challenge_response);
            status = CNGW_Handshake_STATUS_SUCCESS;
        }
        else
        {
            ESP_LOGE(TAG, "HMAC validation failed: %s", esp_err_to_name(HMAC_Result));
            unsuccessful_handshake_attempts++;
        }
    }
    else
    {
        HMAC_Result = ESP_OK;
        status = CNGW_Handshake_STATUS_SUCCESS;
    }

    // 4. create the header
    CCP_UTIL_Get_Msg_Header(&gw1.header, CNGW_HEADER_TYPE_Handshake_Response, sizeof(gw1.message));

    // 5. setup the message field
    gw1.message.command = CNGW_Handshake_CMD_GW1;
    gw1.message.status = status;

    // 6. if the incoming HMAC was validated, populate the rest of the meta data accordingly
    if (HMAC_Result == ESP_OK)
    {
        uint8_t gatewaySerial[CNGW_SERIAL_NUMBER_LENGTH] = {'0', '0', '0', '0', '0', '0', '0', '0', '0'};
        memcpy(gw1.message.gateway_serial, gatewaySerial, CNGW_SERIAL_NUMBER_LENGTH);
        gw1.message.gateway_model = 0;
        gw1.message.firmware_version = *GWVer_Get_Firmware();
        gw1.message.bootloader_version = *GWVer_Get_Bootloader_Firmware();
    }

    // 7. calculate the response HMAC for the mainboard to verify
    msg_length = sizeof(gw1.message) - sizeof(gw1.message.hmac);
    if (!avoid_HMAC_functions)
    {
        HMAC_Result = ATMEL_HMAC((uint8_t *)&gw1.message, msg_length, 3, gw1.message.hmac);
    }

    // 8. if the HMAC was created, then send the message to the mainboard.
    if (HMAC_Result == ESP_OK)
    {
        consume_GW_message((uint8_t *)&gw1);
    }

    cn_board_info.cn_config.cn_is_handshake = false;
    check_for_GW_availability();
    atleast_one_handshake_attempt_recieved = true;
}

void prepare_CN_handshake_02_feedback(CNGW_Handshake_CN2_Frame_t *cn)
{
    if(print_header_frame_info)
    {
        ESP_LOGI(TAG, "prepare_CN_handshake_02_feedback");
    }

    const CNGW_Handshake_CN2_t *const   cn2      = &cn->message;
    CNGW_Handshake_Status               status   = CNGW_Handshake_STATUS_FAILED;
    CNGW_Handshake_GW2_Frame_t          gw2      = {0};
    // HMAC verification
    
    size_t msg_length;
    msg_length = sizeof(*cn2) - sizeof(cn2->hmac);
    status  = CNGW_Handshake_STATUS_SUCCESS;
    esp_err_t HMAC_Result = ESP_FAIL;
    if (!avoid_HMAC_functions)
    {
        HMAC_Result = ATMEL_Validate_HMAC((const uint8_t *)cn2, msg_length, 3, cn2->hmac);
        if (HMAC_Result == ESP_OK)
        {
            status = CNGW_Handshake_STATUS_SUCCESS;
            unsuccessful_handshake_attempts = 0;
        }
        else
        {
            ESP_LOGE(TAG, "HMAC validation 02 failed: %s", esp_err_to_name(HMAC_Result));
            unsuccessful_handshake_attempts++;
        }
    }

    CCP_UTIL_Get_Msg_Header(&gw2.header, CNGW_HEADER_TYPE_Handshake_Response, sizeof(gw2.message));
    // setup the message field
    gw2.message.command = CNGW_Handshake_CMD_GW2;
    gw2.message.status = status;

    // 7. calculate the response HMAC for the mainboard to verify
    msg_length = sizeof(gw2.message) - sizeof(gw2.message.hmac);
    if (!avoid_HMAC_functions)
    {
        HMAC_Result = ATMEL_HMAC((uint8_t *)&gw2.message, msg_length, 3, gw2.message.hmac);
    }

    // sending the frame
    consume_GW_message((uint8_t *)&gw2);
}

static const CNGW_Firmware_Version_t current_firmware_version =
    {
        .major = MAJOR_VER,
        .minor = MINOR_VER,
        .ci = CI_BLD_NUM,
        .branch_id = GIT_BRCH_ID};

const CNGW_Firmware_Version_t *GWVer_Get_Firmware(void)
{
    return &current_firmware_version;
}
const CNGW_Firmware_Version_t *GWVer_Get_Bootloader_Firmware(void)
{
    return &current_firmware_version;
}

#endif