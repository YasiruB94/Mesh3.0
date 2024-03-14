import main
import globalVars
import json

# ACTION COMMANDS
def test_GW_Action_Fade_256_in_1000():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D602070114A00F2000B7",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(2, 0, 1, 0)

def test_GW_Action_Fade_512_in_1000():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D602070114A00F400042",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(2, 0, 1, 0)

def test_GW_Action_Fade_1024_in_1000():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D602070114A00F8000AF",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(2, 0, 1, 0)


def test_GW_Action_ToggleLight():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D60E0701149001000048",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)

def test_GW_Action_Broadcast_ON():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D601070F0090E1FF00B4",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(1, 0, 1, 0)

def test_GW_Action_Broadcast_OFF():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 220,
        "str": "010900D603070F00900100008B",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(1, 0, 1, 0)

# QUERY RELATED (from drivers, only focusing on DR_06 from the 8 drivers since it is repetitive)
def test_GW_Query_Update_Message_CN_Info():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Update_Message",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)

def test_GW_Query_Update_Message_SW_Info():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Update_Message",
        "val": 1
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)

def test_GW_Query_Update_Message_GW_Info():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Update_Message",
        "val": 2
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)



def test_GW_Query_Update_Message_DR_06_Info():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Update_Message",
        "val": 8
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(5, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)

def test_GW_Query_Update_Message_Wrong_TGT():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Update_Message",
        "val": -1
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 0, 0, 1)

# QUERY GENERAL CONFIG RELATED
def test_GW_Query_General_Config():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "General_Config",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)

# QUERY ATTRIBUTE RELATED (only getting the attribute of channel 21, since there are 32 channels and its repetitive)
def test_GW_Query_Attribute_CH21():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Attribute_Message",
        "val": 20
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)

def test_GW_Query_Attribute_Wrong_CH():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Attribute_Message",
        "val": -1
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 0, 0, 1)

# QUERY CHANNEL STATUS RELATED (only setting the status of channel 21, since there are 32 channels and its repetitive)
def test_GW_Query_Status_CH21():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Channel_Status_Message",
        "val": 20
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 1, 2, 0)
    if(globalVars.controlDataSuccess == 2):
        resultJSON = json.loads(globalVars.successMsgs[1])
        assert ("str" in resultJSON)

def test_GW_Query_Status_Wrong_CH():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Channel_Status_Message",
        "val": -1
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(4, 0, 0, 1)

# QUERY ALL CHANNEL (Returns false for now)
def test_GW_Query_All_CH():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 221,
        "str": "Query_All_Channel",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert_type_02(2, 0, 0, 1)

# OTA FW RELATED

def test_GW_Cence_OTA_SW_Older_FW():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_sw_mcu-v2.5.18.0.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(10, 0, 1, 2)

def test_GW_Cence_OTA_CN_Older_FW():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu-v2.5.0.0.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(30, 0, 1, 2)


def test_GW_Cence_OTA_SW():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_sw_mcu-v2.5.19.0.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(30, 0, 3, 0)


def test_GW_Cence_OTA_DR():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_dr_mcu-v2.5.6.0.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(85, 0, 3, 0)


def test_GW_Cence_OTA_CN():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu-v2.5.1.0.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(145, 0, 3, 0)

def test_GW_Cence_OTA_Wrong_URL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "https://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(4, 0, 1, 0)

def test_GW_Cence_OTA_Wrong_URL_02():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.sensorMacs,
        "str": "ht://cence-panel-firmware.s3.us-west-2.amazonaws.com/cense_cn_mcu.bin",
        "val": 1
    }
    main.sendToRoot(data)
    globalVars.timeout_assert_type_02(4, 0, 1, 0)




