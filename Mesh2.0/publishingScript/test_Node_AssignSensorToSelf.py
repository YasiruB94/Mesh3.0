import main
import globalVars
import pytest
import time


@pytest.fixture
def TurnOffAutoPairing():
    data = {
        "usrID": "testID",
        "cmnd": 17,
        "str": "",
        "val": 0
    }
    main.sendToNode(data)
    # above teardown triggers success messages, wait for it to arrive and clear
    time.sleep(3)
    globalVars.resetVars()
    return
    # teardown code to run


def test_AssignSensorToSelf(TurnOffAutoPairing):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 5,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_AssignSensorToSelfCorruptSensorMac(TurnOffAutoPairing):
    globalVars.resetVars()
    print("corrupt sensor mac")
    sensorMacCopy = main.sensorMac[:-1]
    data = {
        "usrID": "testID",
        "cmnd": 5,
        "str": sensorMacCopy,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_AssignSensorToSelfExtraAttr(TurnOffAutoPairing):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 5,
        "str": main.sensorMac,
        "val": 0,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)
