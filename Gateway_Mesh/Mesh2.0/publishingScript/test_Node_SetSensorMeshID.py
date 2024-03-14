import main
import globalVars
import json
import time
import pytest


@pytest.fixture
def ResetMeshID():
    yield
    time.sleep(5)
    data = {
        "usrID": "testID",
        "cmnd": 18,
        "str": main.sensorMac + "_Reset",
        "val": 0
    }
    main.sendToNode(data)
    time.sleep(5)
    globalVars.resetVars()


def test_SetSensorMeshID(ResetMeshID):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 18,
        "str": main.sensorMac + "_Test",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_SetSensorMeshIDMissingAttr(ResetMeshID):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 18,
        # "str": "",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)
