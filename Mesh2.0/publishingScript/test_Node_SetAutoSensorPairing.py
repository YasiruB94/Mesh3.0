import main
import globalVars
import json
import time
import pytest


@pytest.fixture
def ResetAutoSensorPairing():
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
    yield
    # teardown code to run


def test_SetAutoSensorPairing(ResetAutoSensorPairing):
    data = {
        "usrID": "testID",
        "cmnd": 17,
        "str": "",
        "val": 1
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_SetAutoSensorPairingMissingAttr(ResetAutoSensorPairing):
    data = {
        "usrID": "testID",
        "cmnd": 17,
        # "str": "",
        "val": 1
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)
