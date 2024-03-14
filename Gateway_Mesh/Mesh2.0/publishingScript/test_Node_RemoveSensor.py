import main
import globalVars
import pytest
import time


@pytest.fixture
def resubSensor():
    yield
    # teardown code to run
    data = {
        "usrID": "testID",
        "cmnd": 5,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    # above teardown triggers success messages, wait for it to arrive and clear
    time.sleep(5)
    globalVars.resetVars()


def test_RemoveSensor(resubSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 7,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_RemoveSensorCorruptSensorMac(resubSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 7,
        "str": main.sensorMac[:-1],
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_RemoveSensorExtraAttr(resubSensor):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 7,
        "str": main.sensorMac,
        "val": 0,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)
