import main
import globalVars
import time
import pytest


@pytest.fixture
def AssignSensor():
    data = {
        "usrID": "testID",
        "cmnd": 5,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    data = {
        "usrID": "testID",
        "cmnd": 17,
        "str": "",
        "val": 0
    }
    main.sendToNode(data)
    # above teardown triggers success messages, wait for it to arrive and clear
    time.sleep(5)
    globalVars.resetVars()
    return
    # teardown code to run


def test_AssignSensorToNOG(AssignSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 8,
        "str": main.sensorMac + "_" + main.macOfNodeWithSensor,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_AssignSensorToNOGInvalidStrNoDelim(AssignSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 8,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)

    print("invalid str (no delimiter)")


def test_AssignSensorToNOGInvalidSensorMac(AssignSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 8,
        "str": main.sensorMac[:-1] + "_" + main.macOfNodeWithSensor,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_AssignSensorToNOGMissingStrVal(AssignSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 8,
        # "str": main.sensorMac + "_" + main.macsOfNodes[0],
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_AssignSensorToNOGExtraAttr(AssignSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 8,
        "str": main.sensorMac + "_" + main.macOfNodeWithSensor,
        "val": 0,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)
