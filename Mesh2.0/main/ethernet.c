#ifdef ROOT
#include "includes/ethernet.h"
#ifdef GATEWAY_ETH
#include "gw_includes/I2C_comm.h"
#endif

#define PIN_SMI_MDC 23
#define PIN_SMI_MDIO 18
#define PIN_PHY_POWER 12

static uint8_t ubyEthernetLinkCounter = 0;

#define ETH_HELPER_CHECK(expect, str, goto_tag, ret_value, ...)               \
    do                                                                        \
    {                                                                         \
        if (!(expect))                                                        \
        {                                                                     \
            ESP_LOGE(TAG, "%s(%d): " str, __func__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                  \
            goto goto_tag;                                                    \
        }                                                                     \
    } while (0);
#define ETH_EXECUTE_CALLBACK(user_cb, cb_1, ...) \
    if ((user_cb)->cb_1)                         \
        (user_cb)->cb_1(__VA_ARGS__);

static const char *TAG = "SpacrEth";

typedef struct
{
    void (*eth_connect_event)(void);
    void (*eth_disconnect_event)(void);
    void (*eth_start_event)(void);
    void (*eth_stop_event)(void);
    void (*got_ip_event)(void);
} eth_helper_t;
static eth_helper_t *g_eth_helper;

#ifdef GATEWAY_ETH
/**
 * @brief Sets the state of the PHY reset pin
 * @param[in]  num   The pin number
 * @param[in]  state true for set pin, false to unset pin
 * @return result of command
 */
esp_err_t PHY_Set_Pin(uint8_t num, bool state)
{
    esp_err_t ret = ESP_OK;
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

#endif


static void phy_device_power_enable_via_gpio(bool enable)
{
    if (!enable)
        phy_lan8720_default_ethernet_config.phy_power_enable(false);
#ifdef GATEWAY_ETH
    PHY_Set_Pin(PHY_RESET_CHANNEL, enable);
#else
    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_PHY_POWER, (int)enable);
#endif
    // Allow the power up/down to take effect, min 300us
    vTaskDelay(1);

    if (enable)
        phy_lan8720_default_ethernet_config.phy_power_enable(true);
}

static void eth_gpio_config_rmii(void)
{
    phy_rmii_configure_data_interface_pins();
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}

/** Event handler for Ethernet events */
static esp_err_t eth_event_handler(void *ctx, system_event_t *event)
{
    tcpip_adapter_ip_info_t ip;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_ETH_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        ETH_EXECUTE_CALLBACK(g_eth_helper, eth_connect_event);
        ubyEthernetLinkCounter++;
        if (ubyEthernetLinkCounter > 10)
        {
            esp_restart();
        }
        break;

    case SYSTEM_EVENT_ETH_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        ETH_EXECUTE_CALLBACK(g_eth_helper, eth_disconnect_event);
        ubyEthernetLinkCounter++;
        if (ubyEthernetLinkCounter > 10)
        {
            esp_restart();
        }
        break;

    case SYSTEM_EVENT_ETH_START:
        ESP_LOGI(TAG, "Ethernet Started");
        ETH_EXECUTE_CALLBACK(g_eth_helper, eth_start_event);
        break;

    case SYSTEM_EVENT_ETH_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        ETH_EXECUTE_CALLBACK(g_eth_helper, eth_stop_event);
        free(g_eth_helper);
        g_eth_helper = NULL;
        ubyEthernetLinkCounter++;
        if (ubyEthernetLinkCounter > 10)
        {
            esp_restart();
        }
        break;

    case SYSTEM_EVENT_ETH_GOT_IP:
        memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
        ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(ESP_IF_ETH, &ip));
        ESP_LOGI(TAG, "Ethernet Got IP Address");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip.ip));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ETH_EXECUTE_CALLBACK(g_eth_helper, got_ip_event);
        ubyEthernetLinkCounter = 0;
        break;

    default:
        break;
    }

    return ESP_OK;
}

esp_err_t eth_init(eth_helper_init_t *eth_helper_init_struct)
{
    esp_err_t ret = ESP_OK;
    ETH_HELPER_CHECK(eth_helper_init_struct != NULL, "eth_helper_init_struct should not be NULL", direct_ret, ESP_ERR_INVALID_ARG)
    ETH_HELPER_CHECK(g_eth_helper == NULL, "g_eth_helper should only be initilized once", direct_ret, ESP_ERR_INVALID_STATE);
    ETH_HELPER_CHECK((g_eth_helper = (eth_helper_t *)calloc(1, sizeof(eth_helper_t))) != NULL, "memory is not enough", direct_ret, ESP_ERR_NO_MEM);
    g_eth_helper->got_ip_event = eth_helper_init_struct->got_ip;
    g_eth_helper->eth_connect_event = eth_helper_init_struct->eth_connect_event;
    g_eth_helper->eth_disconnect_event = eth_helper_init_struct->eth_disconnect_event;
    g_eth_helper->eth_start_event = eth_helper_init_struct->eth_start_event;
    g_eth_helper->eth_stop_event = eth_helper_init_struct->eth_stop_event;

    ETH_HELPER_CHECK((ret = esp_event_loop_init(eth_event_handler, NULL)) == ESP_OK, " %s", free_before_ret, ret, esp_err_to_name(ret));
#ifdef GATEWAY_ETH
    eth_config_t config = DEFAULT_ETHERNET_PHY_CONFIG;
    config.phy_addr = CONFIG_PHY_ADDRESS;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = 1;
    config.phy_power_enable = phy_device_power_enable_via_gpio;
#else
    eth_config_t config = phy_lan8720_default_ethernet_config;
    config.phy_addr = 0;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = ETH_CLOCK_GPIO17_OUT;
    config.phy_power_enable = phy_device_power_enable_via_gpio;
#endif

    ETH_HELPER_CHECK((ret = esp_eth_init(&config)) == ESP_OK, " %s", free_before_ret, ret, esp_err_to_name(ret));
    ETH_HELPER_CHECK((ret = esp_eth_enable()) == ESP_OK, " %s", free_before_ret, ret, esp_err_to_name(ret));

direct_ret:
    return ret;

free_before_ret:
    free(g_eth_helper);
    g_eth_helper = NULL;
    return ret;
}

esp_err_t eth_deinit()
{
    esp_err_t ret = ESP_OK;
    ETH_HELPER_CHECK(g_eth_helper != NULL, "eth_helper has not been initialized", direct_ret, ESP_ERR_INVALID_STATE);
    ETH_HELPER_CHECK((ret = esp_eth_deinit()) == ESP_OK, " %s", direct_ret, ret, esp_err_to_name(ret));
direct_ret:
    return ret;
}

#endif