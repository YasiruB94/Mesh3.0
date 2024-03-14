#include "mdf_common.h"
#include "mwifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mupgrade.h"
#include "esp_now.h"
#include "math.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "string.h"
#include "driver/gpio.h"
#include "tcpip_adapter.h"
#include "eth_phy/phy_lan8720.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "rom/crc.h"

#define nvsStorage "spacrStore"
#define DISABLE_CHANNEL_SWITCH 1
#define DEFINED_MESH_CHANNEL 11

// DEVICE STATUS DEFINITIONS
#define DEVICE_STATUS_DEAD 0
#define DEVICE_STATUS_ALIVE 1
#define DEVICE_STATUS_RESTARTING 2
#define DEVICE_STATUS_UPDATE_SUCCESS 3
#define DEVICE_STATUS_UPDATE_FAIL 4

extern char deviceMACStr[18];
extern char upgradeFileName[32];
extern size_t upgradeFileSize;

extern void Utilities_InitializeNVS();
extern void Utilities_GetDeviceMac(char *macStr);
extern bool Utilities_StringToMac(const char *strmac, uint8_t *mac);
extern bool Utilities_MacToString(const uint8_t *mac, char *strmac, size_t size);
extern bool Utilities_MacToStringShortVersion(const uint8_t *mac, char *strmac, size_t size);
extern bool Utilities_StringToMacShortVersion(const char *strmac, uint8_t *mac);
extern bool Utilities_ValidateHexString(char *grpID, int length);
extern bool Utilities_ValidateMacAddress(char *macAdd);
extern void print_system_info_timercb(void *timer);
extern bool addrs_remove(uint8_t *addrs_list, size_t *addrs_num, const uint8_t *addr);
