#include "includes/root_utilities.h"
#include "includes/mesh_connection.h"
#include "includes/ethernet.h"
#include "includes/utilities.h"
#include "includes/node_utilities.h"
#include "includes/aws.h"
#include "includes/esp_now_utilities.h"
#include "cJSON.h"

static const char *TAG = "MeshConnection";

EventGroupHandle_t ethernet_event_group;
int ethernet_bit;

#ifdef ROOT
static node_list_t node_list;

void ProcessNodeAddition()
{

    /**
     * @brief find added node.
     */
    // get routing table size and allocate - this will be the new last_list
    node_list.change_num = esp_mesh_get_routing_table_size();
    node_list.change_list = MDF_MALLOC(node_list.change_num * sizeof(mesh_addr_t));
    ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)node_list.change_list,
                                               node_list.change_num * sizeof(mesh_addr_t), (int *)&node_list.change_num));

    // loop through the old table and deduct from the new table
    // to get the added nodes
    for (int i = 0; i < node_list.last_num; ++i)
    {
        addrs_remove(node_list.change_list, &node_list.change_num, node_list.last_list + i * MWIFI_ADDR_LEN);
    }

    // realloc the old table to add the new nodes
    node_list.last_list = MDF_REALLOC(node_list.last_list,
                                      (node_list.change_num + node_list.last_num) * MWIFI_ADDR_LEN);
    memcpy(node_list.last_list + node_list.last_num * MWIFI_ADDR_LEN,
           node_list.change_list, node_list.change_num * MWIFI_ADDR_LEN);
    node_list.last_num += node_list.change_num;

    MDF_LOGI("total_num: %d, add_num: %d", node_list.last_num, node_list.change_num);
    char stringMac[18];
    for (int i = 0; i < (node_list.change_num); i++)
    {
        uint8_t *eventMac = node_list.change_list + (i * MWIFI_ADDR_LEN);
        Utilities_MacToString(eventMac, stringMac, 18);
        if (memcmp(stringMac, deviceMACStr, 6) != 0)
        {
            MDF_LOGI("NEW NODE CONNECTED WITH ADDRESS: %s ", stringMac);
            RootUtilities_SendStatusUpdate(stringMac, DEVICE_STATUS_ALIVE);
        }
    }
    MDF_FREE(node_list.change_list);
}

void ProcessNodeRemoval()
{
    /**
     * @brief find removed node.
     */
    // get routing table size and allocate - this will be the new last_list.
    size_t table_size = esp_mesh_get_routing_table_size();
    uint8_t *routing_table = MDF_MALLOC(table_size * sizeof(mesh_addr_t));
    ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                                               table_size * sizeof(mesh_addr_t), (int *)&table_size));

    // loop through the current table and deduct from the old table
    //  to get the removed nodes
    for (int i = 0; i < table_size; ++i)
    {
        addrs_remove(node_list.last_list, &node_list.last_num, routing_table + i * MWIFI_ADDR_LEN);
    }

    // get the number of changed nodes and the list of changed nodes
    // from the resulting old table
    node_list.change_num = node_list.last_num;
    if (node_list.change_num != 0)
        node_list.change_list = MDF_MALLOC(node_list.change_num * MWIFI_ADDR_LEN);
    memcpy(node_list.change_list, node_list.last_list, node_list.change_num * MWIFI_ADDR_LEN);

    node_list.last_num = table_size;
    memcpy(node_list.last_list, routing_table, table_size * MWIFI_ADDR_LEN);
    MDF_FREE(routing_table);

    MDF_LOGI("total_num: %d, left_num: %d", node_list.last_num, node_list.change_num);
    char stringMac[18];
    for (int i = 0; i < (node_list.change_num); i++)
    {
        uint8_t *eventMac = node_list.change_list + (i * MWIFI_ADDR_LEN);
        Utilities_MacToString(eventMac, stringMac, 18);
        if (memcmp(stringMac, deviceMACStr, 6) != 0)
        {
            MDF_LOGI("NODE DISCONNECTED WITH ADDRESS: %s ", stringMac);
            RootUtilities_SendStatusUpdate(stringMac, DEVICE_STATUS_DEAD);
        }
    }
    MDF_FREE(node_list.change_list);
    // MDF_FREE(node_list.last_list);
}
#endif

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);
    // The CTX is the event information for the mesh!
#ifdef IPNODE
    mesh_event_info_t *event_info = (mesh_event_info_t *)ctx;
#endif

    switch (event)
    {
    case MDF_EVENT_MWIFI_STARTED:
        MDF_LOGI("MESH is started");
        break;
#ifdef ROOT
    case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());
        ProcessNodeAddition();
        break;

    case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
        MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());
        ProcessNodeRemoval();
        break;

    case MDF_EVENT_MWIFI_ROOT_GOT_IP:
        MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");
        xEventGroupSetBits(ethernet_event_group, ethernet_bit);
        break;
#endif
    case MDF_EVENT_MUPGRADE_STARTED:
    {
        mupgrade_status_t status = {0x0};
        mupgrade_get_status(&status);
        MDF_LOGI("MDF_EVENT_MUPGRADE_STARTED, name: %s, size: %d",
                 status.name, status.total_size);
        strcpy(upgradeFileName, status.name);
        upgradeFileSize = status.total_size;
        break;
    }
    case MDF_EVENT_MUPGRADE_STATUS:
        MDF_LOGI("Upgrade progress: %d%%", (int)ctx);
        break;

#ifdef IPNODE
    case MDF_EVENT_MWIFI_PARENT_CONNECTED:
        MDF_LOGI("Parent is connected on station interface");
        rootReachable = true;
        xEventGroupSetBits(ethernet_event_group, ethernet_bit);
        break;

    case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
        MDF_LOGI("Parent is disconnected on station interface");
        rootReachable = false;
        break;

    case MDF_EVENT_MWIFI_NETWORK_STATE:
        if (event_info->network_state.is_rootless)
        {
            rootReachable = false;
        }
        else
        {
            rootReachable = true;
        }
        break;

    case MDF_EVENT_MUPGRADE_FINISH:
        if (strcmp(upgradeFileName, NODEFW_FILE_NAME) == 0)
        {
            // the filename is node firmware
            MDF_LOGI("NODE UPDATE COMPLETED");
        }
        else
        {
            MDF_LOGI("SENSOR UPDATE COMPLETED");
            esp_ota_set_boot_partition(esp_ota_get_running_partition());
        }
#endif
        break;
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
    esp_event_loop_create_default();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_APSTA)); // For Root and Intermediate Parent Nodes it is STA+softAP mode
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    // espnow_init is placed before esp_wifi_start so that we can receive ESPNOW messages once the node is alive
    espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
    espnow_config.qsize.data = 64;
    espnow_init(&espnow_config);
    MDF_ERROR_ASSERT(esp_wifi_start());
    // esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

    return MDF_OK;
}
#endif

#ifdef ROOT
#ifndef GATEWAY_SIM7080
static void eth_got_ip_cb()
{
    mdf_event_loop_send(MDF_EVENT_MWIFI_ROOT_GOT_IP, NULL);
}
#endif

static mdf_err_t wifi_eth_init()
{
    // The NVS stuff is not needed since we initialising and checking in the main.c file
#ifdef GATEWAY_SIM7080
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));
    esp_event_loop_create_default();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_APSTA)); // For Root and Intermediate Parent Nodes it is STA+softAP mode
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());
#else
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
#endif

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
        .channel_switch_disable = DISABLE_CHANNEL_SWITCH, // Changed this to 1, so it disables any channel switch
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
#else
    MDF_ERROR_ASSERT(wifi_init());
#endif
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());
}
