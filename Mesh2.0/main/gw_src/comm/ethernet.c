/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: ethernet.c
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: Legacy code for NODE acting as a ROOT. This code is not being used now
 ******************************************************************************
 *
 ******************************************************************************
 */
#ifdef IPNODE
#include "gw_includes/node_ethernet.h"
static const char *TAG = "node_ethernet";

void ping_task(void *pvParameters)
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct in_addr *addr;
    int s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    vTaskDelay(6000 / portTICK_PERIOD_MS);
    ESP_LOGW(TAG, "PING task begin");
    if (getaddrinfo(DEST_HOST, NULL, &hints, &res) != 0)
    {
        printf("DNS lookup failed for %s\n", DEST_HOST);
        vTaskDelete(NULL);
    }

    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;

    char *ip_address = inet_ntoa(*addr);
    printf("IP Address for %s: %s\n", DEST_HOST, ip_address);

    freeaddrinfo(res);

    while (1)
    {

        ESP_LOGI(TAG, "Opening socket to %s:%d", ip_address, DEST_PORT);
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        {
            printf("Failed to allocate socket.\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(ip_address);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(DEST_PORT);

        if (connect(s, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
        {
            printf("Socket connect failed.\n");
            close(s);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "Connected to %s:%d", ip_address, DEST_PORT);
        // HTTP GET request
        const char *get_request = "GET / HTTP/1.1\r\n"
                                  "Host: www.google.com\r\n"
                                  "Connection: close\r\n\r\n";
        write(s, get_request, strlen(get_request));

        // Read and print the HTTP response status
        char buffer[256];
        int read_len = read(s, buffer, sizeof(buffer) - 1);
        if (read_len > 0)
        {
            buffer[read_len] = '\0';
            ESP_LOGW(TAG, "HTTP Response:\n%s", buffer);
        }
        close(s);

        // Adjust the delay time based on your needs
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Sets the state of the PHY reset pin
 * @param[in]  num   The pin number
 * @param[in]  state true for set pin, false to unset pin
 * @return result of command
 */
esp_err_t PHY_Set_Pin(uint8_t num, bool state)
{
    esp_err_t ret = ESP_FAIL;
    uint8_t pinAddress = PIN0_ON_L + PIN_MULTIPLYER * num;
    if(state)
    {
        ret = PCA9685_Generic_write_i2c_register_two_words(pinAddress & 0xff, 0, 4096);
    }
    else
    {
        ret = PCA9685_Generic_write_i2c_register_two_words(pinAddress & 0xff, 4096, 0);
    }
    return ret;
}

/**
 * @brief gpio specific init
 *
 * @note RMII data pins are fixed in esp32:
 * TXD0 <=> GPIO19
 * TXD1 <=> GPIO22
 * TX_EN <=> GPIO21
 * RXD0 <=> GPIO25
 * RXD1 <=> GPIO26
 * CLK <=> GPIO0
 *
 */
static void eth_gpio_config_rmii(void)
{
    phy_rmii_configure_data_interface_pins();
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}



/**
 * @brief event handler for ethernet
 *
 * @param ctx
 * @param event
 * @return esp_err_t
 */
static esp_err_t eth_event_handler_node(void *ctx, system_event_t *event)
{
    tcpip_adapter_ip_info_t ip;

    switch (event->event_id) {
    case SYSTEM_EVENT_ETH_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case SYSTEM_EVENT_ETH_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
        ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip));
        ESP_LOGI(TAG, "Ethernet Got IP Addr");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip.ip));
        ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip.netmask));
        ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip.gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        break;
    case SYSTEM_EVENT_ETH_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
    
    return ESP_OK;
}

esp_err_t Node_Ethernet_Init()
{   esp_err_t result;
    result = PHY_Set_Pin(PHY_RESET_CHANNEL, false);
    vTaskDelay(5 / portTICK_RATE_MS);
    result = PHY_Set_Pin(PHY_RESET_CHANNEL, true);

    tcpip_adapter_init();
    result = esp_event_loop_init(eth_event_handler_node, NULL);
    ESP_LOGI(TAG, "esp_event_loop_init status: %s", esp_err_to_name(result));


    eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = 1;
    
    ESP_ERROR_CHECK(esp_eth_init(&config));

    ESP_ERROR_CHECK(esp_eth_enable()) ;

    xTaskCreate(&ping_task, "ping_task", 2048, NULL, 5, NULL);


    return ESP_OK;
}
#endif