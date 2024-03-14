/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: ota_agent_core.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: Code involved in sending FW data to CENCE mainboard.
 * 
 * NOTE: If GATEWAY_ETH, no bundling occurs, and 20ms delay each 100 packets
 ******************************************************************************
 *
 ******************************************************************************
 */
#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)

#include "gw_includes/ota_agent_core.h"
#include "gw_includes/ccp_util.h"
#include "esp_task_wdt.h"

static const char *TAG = "ota_agent_core";
// ROOT-> NODE OTA variables
uint32_t                    ota_agent_core_total_received_data_len      = 0;
uint32_t                    ota_agent_core_target_address               = 0;
uint32_t                    ota_agent_core_target_start_address         = 0;
uint32_t                    ota_agent_core_data_length                  = 0;
CNGW_Firmware_Binary_Type   ota_agent_core_target_MCU;
uint8_t                     ota_agent_core_major_version                = 0;
uint8_t                     ota_agent_core_minor_version                = 0;
uint8_t                     ota_agent_core_CI_version                   = 0;
uint8_t                     ota_agent_core_branch_ID                    = 0;
bool                        ota_agent_core_OTA_in_progress              = false;
bool                        ota_agent_core_OTA_FW_accepted_by_CN        = false;
uint16_t                    ota_agent_core_wait_time                    = 13500;
// NODE->CN variables
const uint8_t               *ota_agent_core_current_ptr;
uint16_t                    ota_agent_core_block_count                  = 0;
SemaphoreHandle_t           ota_agent_core_OTA_state_semaphore;
uint8_t                     ota_agent_core_number_of_bundled_packets    = 0;
bool                        ota_agent_core_bundle_OTA_packets           = false;
bool                        ota_agent_core_OTA_restart_required_by_CN   = false;
const esp_partition_t       *ota_agent_core_update_partition;

#ifdef GW_DEBUGGING
static bool                 print_all_frame_info                        = true;
static bool                 always_bundle_OTA_packets                   = false;
#else
static bool                 print_all_frame_info                        = false;
static bool                 always_bundle_OTA_packets                   = false;
#endif


/**How long GW will wait for the last status message from CN. This is after the entire
 * binary was sent*/
static const uint32_t LAST_STATUS_MESSAGE_TIMEOUT_TICKS = pdMS_TO_TICKS(500u);

struct OTA_CONTEXT
{
    OTA_STATE state;
    OTA_SESSION_t session;
    OTA_BINARY_t binary;
    QueueHandle_t status_mailbox;
};
typedef struct OTA_CONTEXT OTA_CTX_t;
static OTA_CTX_t ctx = {0};

/**
 * @brief Reset the session timer
 */
static inline void Reset_Session_Timer(void)
{
    vTaskSetTimeOutState(&ctx.session.timer);
    ctx.session.adjust_timeout_ticks = ctx.session.init_timeout_ticks;
}
/**
 * @brief Cleans all the OTA context safely
 */
static inline void Session_Reset(void)
{
    ctx.state = OTA_STATE_INACTIVE;
    Reset_Session_Timer();
    ctx.session.total_binary_size = 0;
    ctx.binary.written_bytes = 0;
    ctx.binary.state = OTABIN_STATE_DIRTY;
}

/**
 * @brief Get the file header from the downloaded binary
 *        by reference
 * @return OTA_FILE_HEADER_t
 */
static inline const OTA_FILE_HEADER_t *Get_File_Header(void)
{
    const uint8_t *dist_payload = &(ctx.binary.buffer.mem[sizeof(CNGW_Ota_Cyrpto_Info_t)]); /*Skip These fields*/
    const OTA_FILE_HEADER_t *header = (const OTA_FILE_HEADER_t *)dist_payload;
    return header;
}

/**
 * @brief Initialize the OTA API. Must only be called once
 * @param session_timeout_ticks[in] Timeout in RTOS ticks
 */
void OTA_Init(const uint32_t session_timeout_ticks)
{
    ota_agent_core_OTA_state_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(ota_agent_core_OTA_state_semaphore);
    Session_Reset();
    ctx.session.init_timeout_ticks = session_timeout_ticks;
    ctx.session.adjust_timeout_ticks = portMAX_DELAY; /*Timer not started*/
    ctx.status_mailbox = xQueueCreate(1, sizeof(CNGW_Ota_Status_Message_t));
}
/**
 * @brief Helper method to get the status message out of the mailbox.
 * @param message[in] A valid memory space to copy the message into
 * @param timeout_ticks[out] Rtos timeout in ticks to wait on the message
 * @return GW_STATUS_GENERIC_SUCCESS, GW_STATUS_GENERIC_TIMEOUT
 */
static inline GW_STATUS Get_Ota_Status_Message(CNGW_Ota_Status_Message_t *const message, const uint32_t timeout_ticks)
{

    if (pdTRUE == xQueueReceive(ctx.status_mailbox, message, timeout_ticks))
    {
        return GW_STATUS_GENERIC_SUCCESS;
    }

    return GW_STATUS_GENERIC_TIMEOUT;
}

/**
 * @brief Main function which handles OTA FW to mainboard. The function exits after the FW is sent to mainboard regardless of the outcome
 * @return GW_STATUS OK if the OTA to mainboard was successful. GW_STATUS FAIL if not
 */
GW_STATUS OTA_Send_Binary(Binary_Data_Pkg_Info_t binary_file)
{
    // define function-specific structs
    typedef union OTA_MESSAGES
    {
        CNGW_Ota_Info_Frame_t        ota_info;
        CNGW_Ota_Binary_Data_Frame_t ota_binary;
    } OTA_MESSAGES_t;

    typedef enum OTA_STATES
    {
        OTA_STATE_FILE_INFO,
        OTA_STATE_PACKAGE_INFO,
        OTA_STATE_CYRPTO_INFO,
        OTA_STATE_BINARY_INFO,
        OTA_STATE_RESET,
    } OTA_STATES;

    // define structs being used in the function
    OTA_DIST_BIN_CTX_t dist_bin                 = {0};
    OTA_MESSAGES_t msg                          = {0};
    CNGW_Ota_Status_Message_t ota_status_msg    = {0};
    // Clear any pending status messages
    Get_Ota_Status_Message(&ota_status_msg, 0);
    // Get the file header
    dist_bin.file_header                        = Get_File_Header();
    // Get and set the begining and current tick counts
    TickType_t beginningTick                    = xTaskGetTickCount();
    TickType_t currentTick                      = xTaskGetTickCount();
    // define and set generic OTA variables
    ota_agent_core_current_ptr                  = binary_file.initial_ptr;
    uint8_t *cur_buf_pos                        = ctx.binary.buffer.mem;
    uint8_t *end_buf_pos                        = &(ctx.binary.buffer.mem[ctx.binary.written_bytes - 1u]);
    GW_STATUS ota_status                        = GW_STATUS_OTA_SAVE_ERR;
    OTA_STATES state                            = OTA_STATE_RESET;
    uint8_t is_timedout                         = 0;
    uint8_t is_stop                             = 0;
    ota_agent_core_OTA_restart_required_by_CN   = false;
    ota_agent_core_OTA_FW_accepted_by_CN        = false;
    uint16_t timeout_delay = 200;

    // since the first OTA packet for a config file require the flash memory be cleared and this takes time, the timeout delay for a config file OTA is altered
    if(binary_file.binary_type == CNGW_FIRMWARE_BINARY_TYPE_config)
    {
        timeout_delay = 2000;
    }

    do
    {
        // reset required variables in the beginning of the loop
        ota_status                              = GW_STATUS_GENERIC_SUCCESS;
        currentTick                             = xTaskGetTickCount();
        // max allowed ticks before timeout occur is 200
        if (xSemaphoreTake(ota_agent_core_OTA_state_semaphore, timeout_delay) != pdTRUE || ota_agent_core_OTA_restart_required_by_CN)
        {
            ESP_LOGW(TAG, "Restarting OTA message sending...");
            Session_Reset();
            ota_agent_core_current_ptr                  = binary_file.initial_ptr;
            cur_buf_pos                                 = ctx.binary.buffer.mem;
            end_buf_pos                                 = &(ctx.binary.buffer.mem[ctx.binary.written_bytes - 1u]);
            state                                       = OTA_STATE_RESET;
            ota_agent_core_OTA_restart_required_by_CN   = false;
            ota_agent_core_OTA_FW_accepted_by_CN        = false;
        }

        // the max required timeout is 8400 (for CN FW). double this time and keep a buffer of 200 ticks
        if (currentTick - beginningTick > 17000)
        {
            is_timedout = 1;
        }

        // check if the receiving data from mainboard has issues. If so, exit loop with errors
        if (cn_message_queue_error)
        {
            is_timedout = 1;
        }

        switch (state)
        {
        case OTA_STATE_RESET:
        {
            ESP_LOGW(TAG, "OTA_STATE_RESET. FW version: %d.%d.X-X", cn_board_info.cn_mcu.application_version.major, cn_board_info.cn_mcu.application_version.minor);
            memset(&dist_bin, 0, sizeof(dist_bin));
            xSemaphoreGive(ota_agent_core_OTA_state_semaphore);
            dist_bin.file_header        = Get_File_Header();
            cur_buf_pos                 = (uint8_t *)dist_bin.file_header;
            state                       = OTA_STATE_FILE_INFO;
            ota_agent_core_block_count  = 0;

            if (always_bundle_OTA_packets)
            {
                ota_agent_core_bundle_OTA_packets = true;
            }
            else
            {
                // check the mainboard FW version
                if (cn_board_info.cn_mcu.application_version.major == 2 && cn_board_info.cn_mcu.application_version.minor < 5)
                {
                    // the FW is old. need to send packets as bundles. Fast OTA process with risks of fail
                    ota_agent_core_bundle_OTA_packets = true;
                    ESP_LOGW(TAG, "Bundling OTA packets ENABLED");
                }
                else
                {
                    // the FW is new. can send packets one at a time. Slow OTA process with reduced risks
                    ota_agent_core_bundle_OTA_packets = false;
                }
            }
#ifdef GATEWAY_ETH
            //cannot bundle packets if it is ETH GW since the spi queue is reduced
            ota_agent_core_bundle_OTA_packets = false;
#endif
        }
        break;
        case OTA_STATE_FILE_INFO:
        {
            if(print_all_frame_info)
            {
                ESP_LOGW(TAG, "OTA_STATE_FILE_INFO");
            }

            // 1. build the header
            const size_t msg_size                                                   = sizeof(msg.ota_info.message.u.binary_count_msg) + 1;
            const size_t msg_len                                                    = msg_size - sizeof(msg.ota_info.message.u.binary_count_msg.crc);
            CCP_UTIL_Get_Msg_Header(&msg.ota_info.header, CNGW_HEADER_TYPE_Ota_Command, msg_size);
            // 2. build the message
            msg.ota_info.message.command                                            = CNGW_OTA_CMD_File_Header_Info;
            msg.ota_info.message.u.binary_count_msg.dist_release_version.major      = binary_file.dist_release_ver.major;
            msg.ota_info.message.u.binary_count_msg.dist_release_version.minor      = binary_file.dist_release_ver.minor;
            msg.ota_info.message.u.binary_count_msg.dist_release_version.ci         = binary_file.dist_release_ver.ci;
            msg.ota_info.message.u.binary_count_msg.dist_release_version.branch_id  = binary_file.dist_release_ver.branch_id;
            msg.ota_info.message.u.binary_count_msg.count                           = 1;
            msg.ota_info.message.u.binary_count_msg.crc                             = CCP_UTIL_Get_Crc8(0, (uint8_t *)(&msg.ota_info.message), msg_len);
            // 3. print out the info
            if(print_all_frame_info)
            {
                ESP_LOGI(TAG, "msg_size: %zu, msg_len:%zu"                          , msg_size, msg_len);
                ESP_LOGI(TAG, "header.command_type: %d"                             , msg.ota_info.header.command_type);
                ESP_LOGI(TAG, "header.crc: %d"                                      , msg.ota_info.header.crc);
                ESP_LOGI(TAG, "header.data_size: %d"                                , msg.ota_info.header.data_size);
                ESP_LOGI(TAG, "message->crc: %d"                                    , msg.ota_info.message.u.binary_count_msg.crc);
                ESP_LOGI(TAG, "message->count: %d"                                  , msg.ota_info.message.u.binary_count_msg.count);
                ESP_LOGI(TAG, "message->dist_release_version.major: %d"             , msg.ota_info.message.u.binary_count_msg.dist_release_version.major);
                ESP_LOGI(TAG, "message->dist_release_version.minor: %d"             , msg.ota_info.message.u.binary_count_msg.dist_release_version.minor);
                ESP_LOGI(TAG, "message->dist_release_version.ci: %d"                , msg.ota_info.message.u.binary_count_msg.dist_release_version.ci);
                ESP_LOGI(TAG, "message->dist_release_version.branch_id: %d"         , msg.ota_info.message.u.binary_count_msg.dist_release_version.branch_id);
                ESP_LOGI(TAG, "binary_count_msg.crc: %d"                            , msg.ota_info.message.u.binary_count_msg.crc);
            }
            // 4. send the frame
            consume_GW_message((uint8_t *)&msg);
            // 5. setup for package header
            dist_bin.binary_count                                                   = 1;
            cur_buf_pos                                                             = (uint8_t *)dist_bin.file_header;
            dist_bin.package_header                                                 = (CNGW_Ota_Binary_Package_Header_t *)&cur_buf_pos[sizeof(*(dist_bin.file_header))];
            cur_buf_pos                                                             = (uint8_t *)dist_bin.package_header;
            dist_bin.file_header                                                    = NULL;
            state                                                                   = OTA_STATE_PACKAGE_INFO;
        }
        break;
        case OTA_STATE_PACKAGE_INFO:
        {
            if(print_all_frame_info)
            {
                ESP_LOGW(TAG, "OTA_STATE_PACKAGE_INFO");
            }
            // 1. build the header
            const size_t msg_size                                                       = sizeof(msg.ota_info.message.u.package_header_msg) + 1;
            const size_t msg_len                                                        = msg_size - sizeof(msg.ota_info.message.u.package_header_msg.crc);
            CCP_UTIL_Get_Msg_Header(&msg.ota_info.header, CNGW_HEADER_TYPE_Ota_Command, msg_size);
            // 2. build the message
            msg.ota_info.message.command                                                = CNGW_OTA_CMD_Package_Header_Info;
            msg.ota_info.message.u.package_header_msg.package_header.type               = binary_file.binary_type;
            msg.ota_info.message.u.package_header_msg.package_header.version.major      = binary_file.binary_ver.major;
            msg.ota_info.message.u.package_header_msg.package_header.version.minor      = binary_file.binary_ver.minor;
            msg.ota_info.message.u.package_header_msg.package_header.version.ci         = binary_file.binary_ver.ci;
            msg.ota_info.message.u.package_header_msg.package_header.version.branch_id  = binary_file.binary_ver.branch_id;
            msg.ota_info.message.u.package_header_msg.package_header.size               = binary_file.binary_size;
            msg.ota_info.message.u.package_header_msg.package_header.crc                = binary_file.binary_full_crc;
            msg.ota_info.message.u.package_header_msg.crc                               = CCP_UTIL_Get_Crc8(0, (uint8_t *)(&msg.ota_info.message), msg_len);
            // 3. print out the info
            if(print_all_frame_info)
            {
                ESP_LOGI(TAG, "message->crc: %d"                                        , msg.ota_info.message.u.package_header_msg.crc);
                ESP_LOGI(TAG, "message->package_header.type: %d"                        , msg.ota_info.message.u.package_header_msg.package_header.type);
                ESP_LOGI(TAG, "message->package_header.version.major: %d"               , msg.ota_info.message.u.package_header_msg.package_header.version.major);
                ESP_LOGI(TAG, "message->package_header.version.minor: %d"               , msg.ota_info.message.u.package_header_msg.package_header.version.minor);
                ESP_LOGI(TAG, "message->package_header.version.ci: %d"                  , msg.ota_info.message.u.package_header_msg.package_header.version.ci);
                ESP_LOGI(TAG, "message->package_header.version.branch_id: %d"           , msg.ota_info.message.u.package_header_msg.package_header.version.branch_id);
                ESP_LOGI(TAG, "message->package_header.size: %d"                        , msg.ota_info.message.u.package_header_msg.package_header.size);
                ESP_LOGI(TAG, "message->package_header.crc: %u"                         , msg.ota_info.message.u.package_header_msg.package_header.crc);
            }
            // 4. build the frame
            consume_GW_message((uint8_t *)&msg.ota_info);
            // 5. setup for crypto header
            dist_bin.binary_size                                                        = binary_file.binary_size_mod;
            cur_buf_pos                                                                 = (uint8_t *)dist_bin.package_header;
            dist_bin.crypto                                                             = (CNGW_Ota_Cyrpto_Info_t *)&cur_buf_pos[sizeof(*(dist_bin.package_header))];
            cur_buf_pos                                                                 = (uint8_t *)dist_bin.crypto;
            dist_bin.package_header                                                     = NULL;
            state                                                                       = OTA_STATE_CYRPTO_INFO;
        }
        break;
        case OTA_STATE_CYRPTO_INFO:
        {
            if(print_all_frame_info)
            {
                ESP_LOGW(TAG, "OTA_STATE_CYRPTO_INFO");
            }

            // 1. for now, setting the crypto data to values of zero
            uint8_t ecdsa   [64];
            uint8_t random  [32];
            uint8_t padding [32];
            memset(ecdsa,   0, sizeof(ecdsa)    );
            memset(random,  0, sizeof(random)   );
            memset(padding, 0, sizeof(padding)  );

            // 2. build the header
            const size_t msg_size                                                       = sizeof(msg.ota_info.message.u.crypto_msg) + 1;
            const size_t msg_len                                                        = msg_size - sizeof(msg.ota_info.message.u.crypto_msg.crc);
            CCP_UTIL_Get_Msg_Header(&msg.ota_info.header, CNGW_HEADER_TYPE_Ota_Command, msg_size);
            // 3. build the message
            msg.ota_info.message.command                                                = CNGW_OTA_CMD_Crypto_Info;
            memcpy(msg.ota_info.message.u.crypto_msg.cyrpto.ecdsa,      ecdsa,          sizeof(ecdsa));
            memcpy(msg.ota_info.message.u.crypto_msg.cyrpto.random,     random,         sizeof(random));
            memcpy(msg.ota_info.message.u.crypto_msg.cyrpto.padding,    padding,        sizeof(padding));
            msg.ota_info.message.u.crypto_msg.crc                                       = CCP_UTIL_Get_Crc8(0, (uint8_t *)(&msg.ota_info.message), msg_len);

            // 4. build the frame
            consume_GW_message((uint8_t *)&msg.ota_info);

            // 5. setup for binary data
            cur_buf_pos                                                                 = (uint8_t *)dist_bin.crypto;
            dist_bin.encyrpt_binary                                                     = &cur_buf_pos[sizeof(*(dist_bin.crypto))];
            cur_buf_pos                                                                 = (uint8_t *)dist_bin.encyrpt_binary;
            dist_bin.crypto                                                             = NULL;
            state                                                                       = OTA_STATE_BINARY_INFO;
        }
        break;
        case OTA_STATE_BINARY_INFO:
        {
            // 1. initialize and set function specific variables
            uint8_t binary[128];
            memset(binary, 0, sizeof(binary));

            // 2. build the header
            const size_t max_binary_size                                                = sizeof(msg.ota_binary.message.encrypted_binary_and_crc) - 1;
            const int32_t write_bytes                                                   = MIN((int32_t)max_binary_size, dist_bin.binary_size);
            uint8_t *const crc_pos                                                      = &(msg.ota_binary.message.encrypted_binary_and_crc[write_bytes]);
            const size_t msg_size                                                       = sizeof(msg.ota_binary.message.command) + write_bytes + sizeof(*crc_pos);
            const size_t msg_len                                                        = msg_size - sizeof(*crc_pos);
            CCP_UTIL_Get_Msg_Header(&msg.ota_binary.header, CNGW_HEADER_TYPE_Ota_Command, msg_size);

            // 3. build the message
            msg.ota_binary.message.command = CNGW_OTA_CMD_Binary_Data;
            void *read_buffer = malloc(write_bytes);
            if (read_buffer == NULL)
            {
                // failed to allocate malloc memory. exit loop
                ESP_LOGE(TAG, "Failed to allocate memory");
                ota_status = GW_STATUS_GENERIC_BAD_PARAM;
                is_stop = 1;
            }
            else
            {
                // read OTA data from required packet
                esp_err_t result = spi_flash_read((uint32_t)ota_agent_core_current_ptr, read_buffer, write_bytes);
                if (result == ESP_OK)
                {
                    memcpy(msg.ota_binary.message.encrypted_binary_and_crc, read_buffer, write_bytes);
                }
                else
                {
                    // failed to read SPI flash memory. exit loop
                    ESP_LOGE(TAG, "SPI failed to read: %s", esp_err_to_name(result));
                    ota_status = GW_STATUS_GENERIC_BAD_PARAM;
                    is_stop = 1;
                }
            }

            free(read_buffer);
            *crc_pos = CCP_UTIL_Get_Crc8(0, (const uint8_t *)&msg.ota_binary.message, msg_len);

            // 4. update the pointer for the next loop
            ota_agent_core_current_ptr += write_bytes;

            // 5. build the frame
            consume_GW_message((uint8_t *)&msg.ota_binary);

            // 6. print out information every 10 packets
            if(print_all_frame_info)
            {
                if (ota_agent_core_block_count % 100 == 0 || is_timedout)
                {
                    ESP_LOGI(TAG, "ota_binary.binary_size left: %" PRId32 ", ota_agent_core_block_count: %d", dist_bin.binary_size, ota_agent_core_block_count);
                }
#ifdef GATEWAY_ETH
                vTaskDelay(20 / portTICK_RATE_MS);
#endif
            }

            // 7. update other variables for the next loop
            ota_agent_core_block_count      ++;
            dist_bin.binary_size            -= write_bytes;
            dist_bin.encyrpt_binary         += write_bytes;

            // 8. IF required, release the semaphore to keep on looping BEFORE mainboard response
            if (ota_agent_core_bundle_OTA_packets)
            {
                // update the amount of bundled packets
                ota_agent_core_number_of_bundled_packets++;

                // the pattern of sending the packets is different according to the MCU target.
                // 8.1. if target is SW, no bundling required

                // 8.2. if target is DR, packets are bundled periodically with a max bundle of 5
                if (ota_agent_core_target_MCU == CNGW_FIRMWARE_BINARY_TYPE_dr_mcu)
                {
                    if (ota_agent_core_block_count > 4)
                    {
                        if (ota_agent_core_number_of_bundled_packets < 6)
                        {
                            xSemaphoreGive(ota_agent_core_OTA_state_semaphore);
                            vTaskDelay(20 / portTICK_PERIOD_MS);
                        }
                    }
                }

                // 8.3. if target is CN, the periodicity of bundling changes according to the block count. the max bundle is 6
                else if (ota_agent_core_target_MCU == CNGW_FIRMWARE_BINARY_TYPE_cn_mcu)
                {
                    if (ota_agent_core_block_count > 3)
                    {
                        if (ota_agent_core_number_of_bundled_packets < 7)
                        {
                            xSemaphoreGive(ota_agent_core_OTA_state_semaphore);
                            if (ota_agent_core_block_count % 350 == 0 && ota_agent_core_block_count > 300)
                            {
                                // Time given to reset the watchdog to avoid error messages
                                vTaskDelay(20 / portTICK_PERIOD_MS);
                            }
                            else
                            {
                                ets_delay_us(ota_agent_core_wait_time);
                            }
                        }
                    }
                }
            }

            // 9. checks if all binary data is sent to the CN 
            if (dist_bin.binary_size <= 0)
            {
                // checks if all the types of bin files are sent to the CN
                // for now, we are sending one type of FW at a time. so, this check is unnecessary (kept for legacy purposes)
                dist_bin.binary_count--;
                if (dist_bin.binary_count > 0)
                {
                    /*Get the next package header*/
                    dist_bin.package_header = (CNGW_Ota_Binary_Package_Header_t *)dist_bin.encyrpt_binary;
                    cur_buf_pos = (uint8_t *)dist_bin.package_header;
                    state = OTA_STATE_PACKAGE_INFO;

                    /*Reset these for good measure*/
                    dist_bin.file_header = NULL;
                    dist_bin.crypto = NULL;
                    dist_bin.encyrpt_binary = NULL;
                    dist_bin.binary_size = 0;
                }
                else
                {
                    // all binaries sent to CN
                    is_stop = 1;
                }
            }
            else
            {
                // keep the current state and continue sending the binary
                // move the current buffer position to perform memory checks
                cur_buf_pos = (uint8_t *)dist_bin.encyrpt_binary;
            }
        }
        break;
        default:
            break;
        }

        // do checks before running loop again
        if ((cur_buf_pos > end_buf_pos) || !(dist_bin.file_header || dist_bin.package_header || dist_bin.crypto || dist_bin.encyrpt_binary))
        {
            state = OTA_STATE_RESET;
        }

    } while (!is_timedout && !is_stop);

    // end of binary sending. checking the reason for the while loop termination
    if (is_timedout)
    {
        // loop exit due to a timeout error. return with an error message
        ESP_LOGE(TAG, "OTA_Send_Binary END GW_STATUS_OTA_SAVE_TIMEOUT_ERR");
        return GW_STATUS_OTA_SAVE_TIMEOUT_ERR;
    }

    else if (GW_STATUS_GENERIC_SUCCESS == ota_status)
    {
        // loop exit due to successfully completing FW sending. return with success message
        ESP_LOGW(TAG, "OTA_Send_Binary END GW_STATUS_GENERIC_SUCCESS");
        ota_status = GW_STATUS_OTA_SAVE_ERR;
        if (GW_STATUS_GENERIC_SUCCESS == Get_Ota_Status_Message(&ota_status_msg, LAST_STATUS_MESSAGE_TIMEOUT_TICKS))
        {
            if (CNGW_OTA_STATUS_Success == ota_status_msg.status)
            {
                ota_status = GW_STATUS_OTA_SAVE_DONE;
            }
        }
    }

    ESP_LOGI(TAG, "OTA_Send_Binary END. Status: %d", ota_status);
    return ota_status;
}

/**
 * @brief frees the OTA semaphore to continue with sending FW to CN
 * if needed, resets the number of bundled packets
 * @param state_transfer[in] should be true if the semaphore is to be freed
 */
void OTA_next_Frame(bool state_transfer)
{
    if (state_transfer)
    {
        ota_agent_core_wait_time = 12000;
    }
    else
    {
        ota_agent_core_wait_time = 16000;
    }
    if (ota_agent_core_OTA_state_semaphore != NULL)
    {
        xSemaphoreGive(ota_agent_core_OTA_state_semaphore);
        if (ota_agent_core_bundle_OTA_packets)
        {
            ota_agent_core_number_of_bundled_packets = 0;
        }
    }
}


void OTA_Restart_Required()
{
    ota_agent_core_OTA_restart_required_by_CN = true;
    xSemaphoreGive(ota_agent_core_OTA_state_semaphore);
}


void OTA_FW_Success()
{
    ota_agent_core_OTA_FW_accepted_by_CN = true;
}











#endif