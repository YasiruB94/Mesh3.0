#GW required changes: Added the GW c files
set(COMPONENT_SRCS "SpacrGateway_commands.c" 
                    "root_operations.c" 
                    "root_commands.c" 
                    "root_utilities.c" 
                    "main.c" 
                    "aws.c" 
                    "utilities.c" 
                    "mesh_connection.c" 
                    "ethernet.c" 
                    "node_utilities.c" 
                    "SpacrContactor_commands.c" 
                    "SpacrIO_commands.c" 
                    "SpacrPower_commands.c" 
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
                    "gw_src/comm/ethernet.c"
                    "gw_src/comm/SIM7080.c"
                    "gw_src/crypto/cense_sha256.c"
                    "gw_src/crypto/cence_crypto_chip_provision.c"
                    "gw_src/misc/cence_crc.c"
                    )

set(COMPONENT_ADD_INCLUDEDIRS ".")

# requirements can't depend on config
set(COMPONENT_REQUIRES aws_iot ethernet mcommon mconfig mwifi MCP4551 esp_https_ota mupgrade driver esp_adc_cal SIM7080)

register_component()


target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca-SIM.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)

######### Devices' definitions
# uncomment the below definition during release to activate the following:
# 1. remove all critical/ un-necessary/ debugging logs
# 2. avoid OTA of similar CN and SW FW. (for DR, there are no checks)
add_compile_definitions(GW_DEBUGGING)
#add_compile_definitions(PROVISION_ATMEL508)
#add_compile_definitions(SIM7080_ADD_CERTS)

######### options: ROOT, ROOT+GATEWAY_ETH, IPNODE, IPNODE+GATEWAY_SIM7080, GATEWAY_SIM7080
#add_compile_definitions(ROOT)
#add_compile_definitions(GATEWAY_ETH)
add_compile_definitions(IPNODE)
#add_compile_definitions(GATEWAY_SIM7080)



