#include "includes/mesh_connection.h"
#include "includes/ethernet.h"
#include "includes/utilities.h"
#include "includes/root_utilities.h"
#include "includes/node_utilities.h"
#include "includes/aws.h"
#include "cJSON.h"

static const char *TAG = "MeshConnection";

EventGroupHandle_t ethernet_event_group;
int ethernet_bit;

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);
    mesh_event_info_t *event_info = (mesh_event_info_t *)ctx;

    switch (event)
    {
    case MDF_EVENT_MWIFI_STARTED:
        MDF_LOGI("MESH is started");
        break;

    case MDF_EVENT_MWIFI_PARENT_CONNECTED:
        MDF_LOGI("Parent is connected on station interface");
        /**< Start DHCP client on station interface for root node */
        if (esp_mesh_is_root())
        {
            tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        }
        else
        {
            tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
        }
        xEventGroupSetBits(ethernet_event_group, ethernet_bit);
        break;

    case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());
        // MDF_LOGI("device joined: %s", mesh_event_child_connected_t->mac[0])
        // MDF_LOGI("device joined: %s", mesh_event_child_connected_t->mac[1])
        break;
    case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
        MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());
        // if (esp_mesh_is_root()){}
        break;

    case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
        MDF_LOGI("Parent is disconnected on station interface");
        break;
    case MDF_EVENT_MWIFI_ROOT_GOT_IP:
        MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");
        xEventGroupSetBits(ethernet_event_group, ethernet_bit);
        break;
    case MDF_EVENT_MUPGRADE_STARTED:
    {
        mupgrade_status_t status = {0x0};
        mupgrade_get_status(&status);
        MDF_LOGI("MDF_EVENT_MUPGRADE_STARTED, name: %s, size: %d",
                 status.name, status.total_size);
        break;
    }
    case MDF_EVENT_MUPGRADE_STATUS:
        MDF_LOGI("Upgrade progress: %d%%", (int)ctx);
        break;
    case MDF_EVENT_MWIFI_CHILD_CONNECTED:
    {

        bool isValidNode = false;
#ifdef ROOT
        isValidNode = esp_mesh_is_root() && mwifi_is_started();
#elif IPNODE
        isValidNode = !esp_mesh_is_root() && mwifi_is_started();
#endif
        if (isValidNode)
        {
            char stringMac[18];
            uint8_t *eventMac = &event_info->child_connected.mac;
            Utilities_MacToString(eventMac, stringMac, 18);
            MDF_LOGI("CHILD CONNECTED WITH ADDRESS: %s ", stringMac);

            cJSON *connectionData = cJSON_CreateObject();
            cJSON_AddItemToObject(connectionData, "devID", cJSON_CreateString(stringMac));
            // 1 for connections and vice versa
            cJSON_AddNumberToObject(connectionData, "status", 1);

            char *payload = cJSON_PrintUnformatted(connectionData);
#ifdef ROOT
            RootUtilities_SendDataToAWS(logTopic, payload);
#elif IPNODE
            NodeUtilities_PrepareJsonAndSendToRoot(60, 0, payload);
#endif
            cJSON_Delete(connectionData);
            free(payload);
        }
        break;
    }

    case MDF_EVENT_MWIFI_CHILD_DISCONNECTED:
    {
        bool isValidNode = false;
#ifdef ROOT
        isValidNode = esp_mesh_is_root() && mwifi_is_started();
#elif IPNODE
        isValidNode = !esp_mesh_is_root() && mwifi_is_started();
#endif
        if (isValidNode)
        {
            char stringMac[18];
            uint8_t *eventMac = &event_info->child_connected.mac;
            Utilities_MacToString(eventMac, stringMac, 18);
            MDF_LOGI("CHILD CONNECTED WITH ADDRESS: %s ", stringMac);

            cJSON *connectionData = cJSON_CreateObject();
            cJSON_AddItemToObject(connectionData, "devID", cJSON_CreateString(stringMac));
            // 0 for disconnections and vice versa
            cJSON_AddNumberToObject(connectionData, "status", 0);

            char *payload = cJSON_PrintUnformatted(connectionData);
            // RootUtilities_SubmitRootMeshConnectionData(payload);
#ifdef ROOT
            RootUtilities_SendDataToAWS(logTopic, payload);
#elif IPNODE
            NodeUtilities_PrepareJsonAndSendToRoot(60, 0, payload);
#endif
            cJSON_Delete(connectionData);
            free(payload);
        }
        break;
    }
    default:
        break;
    }

    return MDF_OK;
}

#ifdef IPNODE
static mdf_err_t wifi_init()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_APSTA)); //For Root and Intermediate Parent Nodes it is STA+softAP mode
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());
    //MDF_ERROR_ASSERT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR));
    //MDF_ERROR_ASSERT(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR));
    // ESP_LOGI(TAG, "This is the protocol %d", WIFI_PROTOCOL_LR);
    // uint8_t lol = 0;
    // esp_wifi_get_protocol(WIFI_IF_AP, &lol);
    // ESP_LOGI(TAG, "This is the protocol %d", lol);
    return MDF_OK;
}
#endif

#ifdef ROOT
static void eth_got_ip_cb()
{
    mdf_event_loop_send(MDF_EVENT_MWIFI_ROOT_GOT_IP, NULL);
}

static mdf_err_t wifi_eth_init()
{
    //The NVS stuff is not needed since we initialising and checking in the main.c file
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    tcpip_adapter_init();
    eth_helper_init_t init_config = {0};
    init_config.got_ip = eth_got_ip_cb;
    MDF_ERROR_ASSERT(eth_init(&init_config));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_APSTA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());
    //MDF_ERROR_ASSERT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR));
    //MDF_ERROR_ASSERT(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR));

    return MDF_OK;
}
#endif

void MeshConnection_MeshSetup()
{

    ethernet_event_group = xEventGroupCreate();
    ethernet_bit = BIT0;

    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config = {
        .channel = DEFINED_MESH_CHANNEL,
        .channel_switch_disable = DISABLE_CHANNEL_SWITCH, //Changed this to 1, so it disables any channel switch
        .mesh_id = MESH_ID,
        .mesh_password = MESH_PASSWORD,
#ifdef ROOT
        .mesh_type = MWIFI_MESH_ROOT
#else
        .mesh_type = MWIFI_MESH_NODE
#endif
    };
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
#ifdef ROOT
    MDF_ERROR_ASSERT(wifi_eth_init());
#endif
#ifdef IPNODE
    MDF_ERROR_ASSERT(wifi_init());
#endif
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());
}