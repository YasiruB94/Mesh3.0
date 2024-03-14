set(COMPONENT_SRCS  "node_utilities.c"  
                    "root_operations.c" 
                    "root_commands.c" 
                    "root_utilities.c" 
                    "main.c" 
                    "aws.c" 
                    "utilities.c" 
                    "mesh_connection.c" 
                    "ethernet.c"                    
                    "node_commands.c" 
                    "esp_now_utilities.c" 
                    "node_operations.c" 
                    "sensor_commands.c"
                    "SpacrGateway_commands.c"
                    "gw_src/cngw_actions/handle_commands.c"
                    "gw_src/misc/ccp_util.c"
                    "gw_src/comm/SPI_comm.c"
                    "gw_src/misc/gpio.c"
                    "gw_src/cngw_actions/handshake.c"
                    "gw_src/cngw_actions/action.c"
                    "gw_src/cngw_actions/query.c"
                    "gw_src/cngw_actions/control.c"
                    "gw_src/cngw_actions/direct_control.c"
                    "gw_src/cngw_actions/logging.c"
                    "gw_src/cngw_actions/channel.c"
                    "gw_src/cngw_actions/ota_agent.c"
                    "gw_src/cngw_actions/ota_agent_core.c"
                    "gw_src/cngw_actions/secondaryUtilities.c"
                    "gw_src/comm/I2C_comm.c"
                    "gw_src/misc/machine_state.c"
                    "gw_src/misc/status_led.c"
                    "gw_src/cngw_actions/configuration.c"
                    "gw_src/crypto/crypto.c"
                    "gw_src/crypto/atecc508a_comm.c"
                    "gw_src/crypto/atecc508a_conf.c"
                    "gw_src/crypto/atecc508a_crc.c"
                    "gw_src/crypto/atecc508a_pwr.c"
                    "gw_src/crypto/atecc508a_sec.c"
                    "gw_src/crypto/atecc508a_util.c"
                    "gw_src/crypto/atecc508a.c"
                    "gw_src/comm/SIM7080.c"
                    "gw_src/crypto/cense_sha256.c"
                    "gw_src/crypto/cence_crypto_chip_provision.c"
                    "gw_src/misc/cence_crc.c"
                    )

set(COMPONENT_ADD_INCLUDEDIRS "includes")
set(COMPONENT_ADD_INCLUDEDIRS "." "gw_includes")

# requirements can't depend on config
set(COMPONENT_REQUIRES aws_iot ethernet mcommon mconfig mwifi MCP4551 esp_https_ota mupgrade driver esp_adc_cal ota ads111x ADS1110 SIM7080)

register_component()

target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca-SIM.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)

# NODEFW_FILE_NAME do NOT!! Change. Anything longer than Mesh causes it to miss out the letters which bricks devices!
# Make sure to name the binary file the same name as "Mesh!"
add_compile_definitions(NODEFW_FILE_NAME="Mesh2_0"
    MESH_ID="Work1"
    MESH_PASSWORD="914E2A2F"
    SENSOR_SECRET="0H5L31M"
    AUTO_SENSOR_PAIRING_DEFAULT=1
    NODE_OUTPUT_PIN_DEFAULT=23
    DEFAULT_SENSOR_ACTION=0
)

### Device definitions
## Uncomment the GATEWAY_DEBUGGING during release to activate the following:
## 1. Remove all critical/ un-necessary/ debugging logs
## 2. Avoid OTA of similar GW, CN and SW FW. (for DR, there are no checks)
add_compile_definitions(GATEWAY_DEBUGGING)
## Uncomment GATEWAY_PROVISION_ATMEL508 to provision the crypto chip
#add_compile_definitions(GATEWAY_PROVISION_ATMEL508)
## Uncomment GATEWAY_SIM7080_ADD_CERTIFICATES to add AWS certificates to the SIM module
#add_compile_definitions(GATEWAY_SIM7080_ADD_CERTIFICATES)

### Device options: ROOT, ROOT+GATEWAY_ETHERNET, ROOT+GATEWAY_SIM7080, IPNODE
## ROOT options
add_compile_definitions(ROOT)
add_compile_definitions(GATEWAY_ETHERNET)
#add_compile_definitions(GATEWAY_SIM7080)

## NODE options (GATEWAY_IPNODE is incomplete)
#add_compile_definitions(IPNODE)
#add_compile_definitions(GATEWAY_IPNODE)

### NOTE:
## 1. The sdkconfig was changed to have PSRAM (to increase heap memory). This was done because GW ROOT has ESP32 WROVER. Be mindful about the ESP32 device you have when compiling the code. you may have to disable PSRAM is it is not supported.
##    To enable/disable PSRAM follow the below commands:
##    idf.py menuconfig -> Component config -> ESP32-specific -> ["if enable: * (press "y"), else: [blank] (press "n")"] Support for external, SPI-connected RAM
##    idf.py menuconfig -> Component config -> ESP32-specific -> Support for external, SPI-connected RAM -> SPI RAM config -> [*] Allow external memory as an arguement to xTaskCreateStatic

