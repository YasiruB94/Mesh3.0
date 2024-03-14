
#include "utilities.h"

#ifdef GATEWAY_ETH
#include "gw_includes/status_led.h"
#define PHY_RESET_CHANNEL 11U 
#define PIN0_ON_L 0x6
#define PIN_MULTIPLYER 4
#define PIN_SMI_MDC 23
#define PIN_SMI_MDIO 18
#define CONFIG_PHY_ADDRESS 1
#define DEFAULT_ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
esp_err_t PHY_Set_Pin(uint8_t num, bool state);
#endif

typedef struct
{
    void (*eth_connect_event)(void);    /*!< eth connect event callback */
    void (*eth_disconnect_event)(void); /*!< eth disconnect event callback */
    void (*eth_start_event)(void);      /*!< eth start event callback */
    void (*eth_stop_event)(void);       /*!< eth stop callback */
    void (*got_ip)(void);               /*!< got ip event callback */
} eth_helper_init_t;                    /*!< eth helper */

extern esp_err_t eth_init(eth_helper_init_t *eth_helper_init_struct);
extern esp_err_t eth_deinit();
