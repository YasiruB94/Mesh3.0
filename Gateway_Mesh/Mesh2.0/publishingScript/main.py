from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import time
import json
from datetime import datetime
import globalVars
import pytest

#### CONFIGURATION ####

# mac address of roots/nodes/groups in standard format:
currentRootMac = "24:0a:c4:26:7a:18"
macOfGroup = "01:00:5e:ae:ae:12"
macsOfNodes = ["e0:e2:e6:7b:df:7c", "c4:dd:57:5b:f4:a8", "7c:87:ce:d0:bd:1c"]
macOfNodeWithSensor = "e0:e2:e6:7b:df:7c"
# macOfNodeWithSensor = "8c:aa:b5:b8:c2:ec"

# mac address of sensor in shortened format (without ':'):
sensorMac = "b8f00993b504"
sensorMacLong = "b8:f0:09:93:b5:04"

# Group ID in string
grpID = "20"

# Organization ID
orgID = "darylTesting"

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

# manual test


def manualTest():

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
    data = {
        "usrID": "testID",
        "cmnd": 19,
        "str": "",
        "val": sensorMacs
    }
    sendToNode(data)
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
retcode = pytest.main(["--disable-pytest-warnings", "-k",
                      "test_UpdateNodeFWValMoreThanMacs"])
# see all output
# retcode = pytest.main(["--disable-pytest-warnings", "--showlocals", "-s"])

# memory leak test
# retcode = pytest.main(["--count=5", "--disable-pytest-warnings"])
