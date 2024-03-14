from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import time
import json
from datetime import datetime
import globalVars
import pytest
#### CONFIGURATION ####

# mac address of roots/nodes/groups in standard format:
old_GW_MAC = "08:3a:8d:15:2c:fc"
SIM_Enabled_GW = "08:d1:f9:59:19:c0"
ETH_Enabled_GW = "08:d1:f9:59:19:ec"

light_level = 0
change_count = 0

Current_USED_GW = ETH_Enabled_GW

currentRootMac = "24:4c:ab:03:fc:b4"
macOfGroup = Current_USED_GW
macsOfNodes = [Current_USED_GW]
macOfNodeWithSensor = Current_USED_GW

# mac address of sensor in shortened format (without ':'):
sensorMac = "08d1f95919c0"
sensorMacLong = Current_USED_GW

# Group ID in string
grpID = "20"

# Organization ID
orgID = "Test_GW"

# URL for OTA software
OTA_ROOT_URL = "https://spacrfirmware.s3.ca-central-1.amazonaws.com/Mesh2_0_root.bin"
OTA_NODE_URL = "https://spacrfirmware.s3.ca-central-1.amazonaws.com/Mesh2_0_node.bin"
OTA_SENSOR_URL = "https://spacrfirmware.s3.ca-central-1.amazonaws.com/SpacrPir.bin"

#### CALCULATIONS ####

numOfNodes = len(macsOfNodes)

# get list of macs in hex
macs = []
for node in macsOfNodes:
    macs.extend(map(lambda f: int(f, 16), node.split(":")))

sensorMacs = []
sensorMacs.extend(map(lambda f: int(f, 16), sensorMacLong.split(":")))
print("SENSORMACS IS: ")
print(sensorMacs)


#### DEFINITIONS ####

# Custom MQTT message callback
def customCallback(client, userdata, message):
    global logsCounter, controlDataSuccess, controlDataFail
    # print("Received a new message: ")
    # print(message.payload)
    # print("from topic: ")
    # print(message.topic)
    # print("--------------\n\n")
    if(message.topic == currentRootMac + "/logs/"):
        globalVars.logsCounter += 1
        globalVars.logsMsgs.append(message.payload)
    elif(message.topic == orgID + "/controldata/success"):
        globalVars.controlDataSuccess += 1
        globalVars.successMsgs.append(message.payload)
    elif(message.topic == orgID + "/controldata/fail"):
        globalVars.controlDataFail += 1
        globalVars.failMsgs.append(message.payload)
    else:
        print("topic unknown: " + message.topic)


def sendToRoot(jsonData):
    messageJson = json.dumps(jsonData)
    myAWSIoTMQTTClient.publish(currentRootMac, messageJson, 1)


def sendToNode(jsonData):
    messageJson = json.dumps(jsonData)
    myAWSIoTMQTTClient.publish(
        currentRootMac + "/" + macOfNodeWithSensor, messageJson, 1)


def sendToGroup(jsonData):
    messageJson = json.dumps(jsonData)
    myAWSIoTMQTTClient.publish(currentRootMac + "/" + macOfGroup, messageJson, 1)


# Read in command-line parameters
endpoint = "a1yr1nntapo4lc-ats.iot.us-west-2.amazonaws.com"
cert = "cert.txt"
key = "key.txt"
port = 8883
useWebsocket = False

clientId = "test546575"
# topicPub = "currentRootMac/mavofroot"
topicSub = "$aws/things/test1/shadow/name/test1/update/delta"

myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId)
myAWSIoTMQTTClient.configureEndpoint(endpoint, port)
myAWSIoTMQTTClient.configureCredentials("root.txt", key, cert)

# AWSIoTMQTTClient connection configuration
myAWSIoTMQTTClient.configureAutoReconnectBackoffTime(1, 32, 20)
# Infinite offline Publish queueing
myAWSIoTMQTTClient.configureOfflinePublishQueueing(-1)
myAWSIoTMQTTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
myAWSIoTMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
myAWSIoTMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec


# Connect and subscribe to AWS IoT
myAWSIoTMQTTClient.connect()


# sub to root topics
myAWSIoTMQTTClient.subscribe(currentRootMac + "/logs/", 0, customCallback)
myAWSIoTMQTTClient.subscribe(orgID + "/controldata/success", 1, customCallback)
myAWSIoTMQTTClient.subscribe(orgID + "/controldata/fail", 1, customCallback)


#control lights
def control_ights(light_lvl):
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "{\"address\": 9 , \"command\": 2, \"address_type\": 1, \"fade_unit\": 0, \"fade_time\": 600}",
        "val": light_lvl
    }
    sendToNode(data)



# manual test

def manualTest():
    print("in the manual test")
    # step 1 load firmware
    # data = {
    #     "usrID": "testID",
    #     "cmnd": 66,
    #     "val": numOfNodes,
    #     "macs": macs,
    #     "str": "https://spacrfirmware.s3.ca-central-1.amazonaws.com/SpacrPir.bin"
    # }
    # sendToRoot(data)
    # step 2 recieve firmware
    """
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D60E0701149001000048",
        "val": 0
    }
    sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)

    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": 1,
        "macs": sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu-v2.5.1.0.bin"

    }
    sendToRoot(data)


    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": 1,
        "macs": sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu-v2.5.3.0.bin"

    }
    sendToRoot(data)
 
    data = {
            "usrID": "testID",
            "cmnd": 55,
            "val": 1,
            "str": "Test_GW"
        }
    sendToRoot(data)

    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": 1,
        "macs": sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_config-v1.4.7.0.bin"

    }
    sendToRoot(data)


#"https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_config-v1.4.7.0.bin",
#"https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_dr_mcu-v2.5.6.0.bin",
#"https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_dr_mcu-v2.5.7.0.bin",
#https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu-v2.5.3.0.bin,
#https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_sw_mcu-v2.5.19.0.bin

    data = {
  "usrID": "testID",
  "cmnd": 220,
  "str": "{\"address\": 20 , \"command\": 14, \"address_type\": 1, \"fade_unit\": 0, \"fade_time\": 100}",
  "val": 100
  }

    sendToNode(data)

"""
"""

    data = {
  "usrID": "testID",
  "cmnd": 221,
  "str": "Change_Driver_Current_DR1CH0",
  "val": 50
  }
    sendToNode(data)
"""
    
# Get_Config_Info
# CENCE_Control
# Change_Driver_Current_DR1CH0
# Restart_Mainboard
"""
data = {
  "usrID": "testID",
  "cmnd": 220,
  "str": "{\"address\": 20 , \"command\": 14, \"address_type\": 1, \"fade_unit\": 0, \"fade_time\": 3000}",
  "val": 30
  }

sendToNode(data)
"""


"""
data = {
        "usrID": "testID",
        "cmnd": 225,

        #Basic settings
        #"str": "{\"AT_cmd\": \"A/\"}",
        #"str": "{\"AT_cmd\": \"AT\"}",
        #"str": "{\"AT_cmd\": \"AT+CGMR\"}",
        #"str": "{\"AT_cmd\": \"AT+CREG=1\"}",
        #"str": "{\"AT_cmd\": \"AT+CREG?\"}",
        #"str": "{\"AT_cmd\": \"AT+COPS?\"}",
        #"str": "{\"AT_cmd\": \"AT+COPS=0,2\"}",
        #"str": "{\"AT_cmd\": \"AT+CGDCONT?\"}",
        #"str": "{\"AT_cmd\": \"AT+CMAR=?\"}",
        #"str": "{\"AT_cmd\": \"AT+CFUN?\"}",
        #"str": "{\"AT_cmd\": \"AT+CRXFTM\"}",


        #"str": "{\"AT_cmd\": \"AT+CPIN?\"}",
        #"str": "{\"AT_cmd\": \"AT+CNMP?\"}",
        #"str": "{\"AT_cmd\": \"AT+CMNB=3\"}",
        #"str": "{\"AT_cmd\": \"AT+CMNB?\"}",
        #"str": "{\"AT_cmd\": \"AT+CSQ\"}",
        #"str": "{\"AT_cmd\": \"AT+CGREG?\"}",
        #"str": "{\"AT_cmd\": \"AT+CGREG=2\"}",
        #"str": "{\"AT_cmd\": \"AT+CGATT?\"}",
        #"str": "{\"AT_cmd\": \"AT+CGDCONT?\"}",
        #"str": "{\"AT_cmd\": \"AT+CGDCONT=1,\\\"IP\\\",\\\"\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+CGATT=0\"}", # this is the code that cmade to select COPS as telus
        #"str": "{\"AT_cmd\": \"AT+COPS?\"}",
        #"str": "{\"AT_cmd\": \"AT+CGNAPN\"}",
        #"str": "{\"AT_cmd\": \"AT+CPSI?\"}",
        #"str": "{\"AT_cmd\": \"AT+CNACT=0,1\"}",
        #"str": "{\"AT_cmd\": \"AT+CNACT?\"}",
        #"str": "{\"AT_cmd\": \"AT+SNPDPID=0\"}",
        #"str": "{\"AT_cmd\": \"AT+SNPING4=\\\"8.8.8.8\\\",1,16,5000\"}",


        # File system
        #"str": "{\"AT_cmd\": \"AT+CFSINIT\"}",
        #"str": "{\"AT_cmd\": \"AT+CFSGFRS?\"}", # get the free size of the file system
        #"str": "{\"AT_cmd\": \"AT+CFSGFIS=3,\\\"rootCA.pem\\\"\"}", # get the file size
        #"str": "{\"AT_cmd\": \"AT+CFSGFIS=3,\\\"deviceCert.crt\\\"\"}", # get the file size
        #"str": "{\"AT_cmd\": \"AT+CFSGFIS=3,\\\"devicePKey.key\\\"\"}", # get the file size

        #"str": "{\"AT_cmd\": \"AT+CFSRFILE=0,\\\"rootCA.pem\\\",0,1759,0\"}", # read file content
        #"str": "{\"AT_cmd\": \"AT+CFSRFILE=0,\\\"deviceCert.crt\\\",0,1241,0\"}", # read file content
        #"str": "{\"AT_cmd\": \"AT+CFSRFILE=0,\\\"devicePKey.key\\\",0,1703,0\"}", # read file content

        #"str": "{\"AT_cmd\": \"AT+CFSDFILE=3,\\\"newroot.pem\\\"\"}", # delete a file
        #"str": "{\"AT_cmd\": \"AT+CFSDFILE=3,\\\"rootCA.pem\\\"\"}", # delete a file
        #"str": "{\"AT_cmd\": \"AT+CFSDFILE=3,\\\"deviceCert.crt\\\"\"}", # delete a file
        #"str": "{\"AT_cmd\": \"AT+CFSDFILE=3,\\\"devicePKey.key\\\"\"}", # delete a file
        #"str": "{\"AT_cmd\": \"AT+CFSTERM\"}",

        #"str": "{\"AT_cert\": \"..\"}",
        #"str": "{\"AT_test\": \"{\\\"usrID\\\": \\\"testID_SIM2\\\"}\"}",
        #"str": "{\"AT_test\": \"{}\"}",

        # MQTT communication (mosquitto)

        #"str": "{\"AT_cmd\": \"AT+SMCONF?\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"CLIENTID\\\", \\\"id\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"KEEPTIME\\\", 60\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"URL\\\", \\\"test.mosquitto.org\\\",\\\"1883\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"CLEANSS\\\", 1\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"QOS\\\", 1\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"TOPIC\\\", \\\"will topic\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"MESSAGE\\\", \\\"will message\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"RETAIN\\\", 1\"}",

        #"str": "{\"AT_cmd\": \"AT+SMCONN\"}", # connect to MQTT
        #"str": "{\"AT_cmd\": \"AT+SMDISC\"}", # disconnect from MQTT
        #"str": "{\"AT_cmd\": \"AT+SMSTATE?\"}", # inquire MQTT connection status (0=disconnected, 1=connected, 2=session present)

        #"str": "{\"AT_cmd\": \"AT+SMSSL=?\"}", # list all SSL certificates
        #"str": "{\"AT_cmd\": \"AT+SMPUB=?\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMPUB=\\\"information\\\",5,1,1\"}", 
        #"str": "{\"AT_cmd\": \"hello\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMSUB=?\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMSUB=\\\"08:d1:f9:59:19:ec/+\\\",1\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMSUB=\\\"testing123\\\",1\"}", 


        ## MQTT communication (AWS) - working

        ## configure MQTT parameters
        #"str": "{\"AT_cmd\": \"AT+SNPING4=a1yr1nntapo4lc.iot.us-west-2.amazonaws.com,1,16,5000\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"URL\\\", a1yr1nntapo4lc.iot.us-west-2.amazonaws.com,8883\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"CLIENTID\\\",08:d1:f9:59:19:c0\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"KEEPTIME\\\",60\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"CLEANSS\\\",1\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"QOS\\\",1\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF?\"}",

        ## configure SSL parameters
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"SSLVERSION\\\",0,3\"}",
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"IGNORERTCTIME\\\",0,1\"}",
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"CONVERT\\\",2,rootCA.pem\"}",
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"CONVERT\\\",1,deviceCert.crt,devicePKey.key\"}",
        
        #"str": "{\"AT_cmd\": \"AT+SMSSL=1,rootCA.pem,deviceCert.crt\"}",
        #"str": "{\"AT_cmd\": \"AT+SMSSL?\"}",

        ## attempt connection
        #"str": "{\"AT_cmd\": \"AT+SMCONN\"}", # connect to MQTT
        #"str": "{\"AT_cmd\": \"AT+SMDISC\"}", # disconnect from MQTT
        #"str": "{\"AT_cmd\": \"AT+SMSTATE?\"}", # inquire MQTT connection status (0=disconnected, 1=connected, 2=session present)

        ## subscribe
        #"str": "{\"AT_cmd\": \"AT+SMSUB=?\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMSUB=\\\"08:d1:f9:59:19:c0/+\\\",1\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMUNSUB=\\\"08:d1:f9:59:19:c0\\\"\"}",

        ## publish
        #"str": "{\"AT_cmd\": \"AT+SMPUB=?\"}", 
        #"str": "{\"AT_cmd\": \"AT+SMPUB=\\\"/controldata/success\\\",5,1,1\"}", 
        #"str": "{\"AT_cmd\": \"hello\"}",




        # MQTT communication (AWS) - legacy
        #"str": "{\"AT_cmd\": \"AT+CNACT?\"}",
        #"str": "{\"AT_cmd\": \"AT+CNACT=0,1\"}",
        #"str": "{\"AT_cmd\": \"AT+SNPING4=a1yr1nntapo4lc.iot.us-west-2.amazonaws.com,1,16,5000\"}",
        #"str": "{\"AT_cmd\": \"AT+CGNAPN\"}",
        #"str": "{\"AT_cmd\": \"AT+CCLK?\"}",# check the system clock
        #"str": "{\"AT_cmd\": \"AT+CCLK=\\\"24/01/12,13:31:00-20\\\"\"}",# set the system clock "yy/MM/dd,hh:mm:ss±zz"
        #"str": "{\"AT_cmd\": \"AT+CNTP?\"}",# check the system clock
        #"str": "{\"AT_cmd\": \"AT+CNTP=\\\"pool.ntp.org\\\",-20,0,2\"}",# set the system clock "yy/MM/dd,hh:mm:ss±zz"

        #"str": "{\"AT_cmd\": \"AT+SMCONF?\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"URL\\\", a1yr1nntapo4lc.iot.us-west-2.amazonaws.com,8883\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"CLIENTID\\\",08:d1:f9:59:19:c0\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"KEEPTIME\\\",60\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"CLEANSS\\\",1\"}",
        #"str": "{\"AT_cmd\": \"AT+SMCONF=\\\"QOS\\\",1\"}",


        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"SSLVERSION\\\",0,3\"}",
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"IGNORERTCTIME\\\",0,1\"}",
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"PROTOCOL\\\",0,1\"}",

        #legacy root
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"CONVERT\\\",2,rootCA.pem\"}",
        #"str": "{\"AT_cmd\": \"AT+CSSLCFG=\\\"CONVERT\\\",1,deviceCert.crt,devicePKey.key\"}",
        #"str": "{\"AT_cmd\": \"AT+SMSSL?\"}",
        #"str": "{\"AT_cmd\": \"AT+SMSSL=1,rootCA.pem,deviceCert.crt\"}",


        #attempt connection
        #"str": "{\"AT_cmd\": \"AT+SMCONN\"}", # connect to MQTT
        #"str": "{\"AT_cmd\": \"AT+SMDISC\"}", # disconnect from MQTT
        #"str": "{\"AT_cmd\": \"AT+SMSTATE?\"}", # inquire MQTT connection status (0=disconnected, 1=connected, 2=session present)


        # HTTP commands
        #"str": "{\"AT_cmd\": \"AT+CGREG?\"}",
        #"str": "{\"AT_cmd\": \"AT+COPS?\"}",
        #"str": "{\"AT_cmd\": \"AT+CGNAPN\"}",
        #"str": "{\"AT_cmd\": \"AT+CNACT?\"}",
        #"str": "{\"AT_cmd\": \"AT+CNACT=0,1\"}", #not working

        
        #"str": "{\"AT_cmd\": \"AT+SHCONF?\"}",
        #"str": "{\"AT_cmd\": \"AT+SHCONF=\\\"URL\\\", \\\"http://www.google.com\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SHCONF=\\\"BODYLEN\\\", 1024\"}",
        #"str": "{\"AT_cmd\": \"AT+SHCONF=\\\"HEADERLEN\\\", 350\"}",
        #"str": "{\"AT_cmd\": \"AT+SHCONN\"}",
        #"str": "{\"AT_cmd\": \"AT+SHSTATE?\"}",
        #"str": "{\"AT_cmd\": \"AT+SHCHEAD\"}",
        #"str": "{\"AT_cmd\": \"AT+SHAHEAD=\\\"Accept\\\", \\\"text/html, */*\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SHAHEAD=\\\"User-Agent\\\", \\\"IOE Client\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SHAHEAD=\\\"Content-Type\\\", \\\"application /x-www-form-urlencoded\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SHAHEAD=\\\"Connection\\\", \\\"keep-alive\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SHAHEAD=\\\"Cache-control\\\", \\\"no-cache\\\"\"}",
        #"str": "{\"AT_cmd\": \"AT+SHREQ=\\\"http://www.google.com/\\\", 1\"}",
        #"str": "{\"AT_cmd\": \"AT+SHREAD=0,21346\"}",

        #"str": "{\"AT_cmd\": \"AT+SHDISC\"}",



        #"str": "{\"AT_cmd\": \"AT+CCLK?\"}",# check the system clock
        #"str": "{\"AT_cmd\": \"AT+CCLK=\\\"24/01/10,12:16:50-20\\\"\"}",# set the system clock "yy/MM/dd,hh:mm:ss±zz"
        "val": 3
    }
sendToNode(data)
"""


    # test sensor commands
    # data = {
    #     "usrID": "testID",
    #     "cmnd": 17,
    #     "str": sensorMac,
    #     "val": 1
    # }
    # sendToNode(data)
    # EXTRA
    # data = {
    #     "usrID": "testID",
    #     "cmnd": 0,
    #     "str": "hello",
    #     "val": 0
    # }

    # sendToNode(data)
    # data = {
    #     "usrID": "testID",
    #     "cmnd": 17,
    #     "str": sensorMac,
    #     "val": 1
    # }
    # sendToNode(data)

    # run manual test
# manualTest()


# full functionality test
# retcode = pytest.main(["--disable-pytest-warnings"])
# specific test (uses pattern matching after -k)
# retcode = pytest.main(["--disable-pytest-warnings", "-k", "test_UpdateNodeFWValMoreThanMacs"])
# retcode = pytest.main(["--disable-pytest-warnings", "-k", "test_GW_Action_Fade_256_in_1000"])
# retcode = pytest.main(["--disable-pytest-warnings", "-k", "test_GW_Cence_OTA_CN"])

#light_level = 100
#manualTest()
# see all output
# retcode = pytest.main(["--disable-pytest-warnings", "--showlocals", "-s"])

# memory leak test
# retcode = pytest.main(["--count=5", "--disable-pytest-warnings"])


light_level = 0
change_count = 0

while(True):
    change_count +=1
    if light_level == 100:
        light_level = 0
    else:
        light_level = 100

    print("setting light level to", light_level,"\tnumber of changes done:", change_count)
    control_ights(light_level)
    time.sleep(8)


