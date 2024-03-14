#include "includes/utilities.h"

static const char *TAG = "Utilities";

void Utilities_InitializeNVS()
{
	// Initialize the NVS
	mdf_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		MDF_ERROR_ASSERT(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	MDF_ERROR_ASSERT(ret);
}

void Utilities_GetDeviceMac(char *macString)
{
	uint8_t mac[6] = {0};
	esp_err_t ret = esp_efuse_mac_get_default(mac);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get base MAC address from EFUSE BLK0. (%s)", esp_err_to_name(ret));
		strcpy(macString, "00:00:00:00:00:00\0");
		//Log to backend
	}
	else
	{
		size_t macSize = 18;
		if (!Utilities_MacToString(mac, macString, macSize))
		{
			strcpy(macString, "00:00:00:00:00:00\0");
		}
	}
}

bool Utilities_StringToMac(const char *strmac, uint8_t *mac)
{
	// validate valid mac exists

	if (6 == sscanf(strmac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Utilities_MacToString(const uint8_t *mac, char *strmac, size_t size)
{
	if (snprintf(strmac, size, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) >= 6)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Utilities_MacToStringShortVersion(const uint8_t *mac, char *strmac, size_t size)
{
	if (snprintf(strmac, size, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) >= 6)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Utilities_StringToMacShortVersion(const char *strmac, uint8_t *mac)
{
	if (strlen(strmac) != 17)
	{
		return false;
	}
	if (6 == sscanf(strmac, "%hhx%hhx%hhx%hhx%hhx%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
	{
		return true;
	}
	else
	{
		return false;
	}
}

// for validating grpID and mac addresses without colon
bool Utilities_ValidateHexString(char *macStr, int length)
{
	errno = 0;
	long long number = 0;
	char *endptr;
	int base = 16;
	number = strtoll(macStr, &endptr, base);
	// checks that the macStr can be parsed as a hexadecimal number, its length is exactly 2, no characters are non-hex, and the groupID is not 0.
	if (errno == 0 && strlen(macStr) == length && (strcmp(endptr, "") == 0) && number != 0)
	{
		return true;
	}
	else
	{
		ESP_LOGI(TAG, "invalid hexadecimal string of length %d", length);
		return false;
	}
}

// for validating mac addresses in standard format e.g. 8c:aa:b5:b8:c2:ec
bool Utilities_ValidateMacAddress(char *macAdd)
{
	const char delim[2] = ":";
	char hexString[13] = "";
	int counter = 0;
	char *token;

	token = strtok(macAdd, delim);

	while (token != NULL)
	{
		ESP_LOGI(TAG, "TOKEN: %s", token);
		if (counter > 5)
		{
			ESP_LOGI(TAG, "Too many colons");
			return false;
		}
		if (!(strlen(token) == 2))
		{
			ESP_LOGI(TAG, "Invalid token");
			return false;
		}
		strcat(hexString, token);
		token = strtok(NULL, delim);
		counter++;
	}

	return Utilities_ValidateHexString(hexString, 12);
}
void print_system_info_timercb(void *timer)
{
	uint8_t primary = 0;
	wifi_second_chan_t second = 0;
	mesh_addr_t parent_bssid = {0};
	uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
	//uint8_t eth_mac[MWIFI_ADDR_LEN] = { 0 };
	wifi_sta_list_t wifi_sta_list = {0x0};

	//esp_eth_get_mac(eth_mac);
	esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
	esp_wifi_ap_get_sta_list(&wifi_sta_list);
	esp_wifi_get_channel(&primary, &second);
	esp_mesh_get_parent_bssid(&parent_bssid);

	if (esp_mesh_is_root() == true)
	{
		MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR
				 ", node num: %d, free heap: %u",
				 primary,
				 esp_mesh_get_layer(), MAC2STR(sta_mac),
				 esp_mesh_get_total_node_num(), esp_get_free_heap_size());
	}
	else
	{
		MDF_LOGI("System information, channel: %d, self mac: " MACSTR
				 ", parent bssid: " MACSTR ", parent rssi: %d, , node num: %d, free heap: %u",
				 primary,
				 MAC2STR(sta_mac), MAC2STR(parent_bssid.addr), mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(),
				 esp_get_free_heap_size());
	}
	for (int i = 0; i < wifi_sta_list.num; i++)
	{
		MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
	}

#ifdef MEMORY_DEBUG

	if (!heap_caps_check_integrity_all(true))
	{
		MDF_LOGE("At least one heap is corrupt");
	}

	mdf_mem_print_heap();
	mdf_mem_print_record();
#endif /**< MEMORY_DEBUG */
}
