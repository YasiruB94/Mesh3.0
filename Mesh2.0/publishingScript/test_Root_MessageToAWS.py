import main
import globalVars
import time


def test_MessageToAWS():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 60,
        "str": "logging",
        "val": 0
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(1, 0, 0)


def test_MessageToAWSExtraProperties():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 60,
        "str": "test",
        "val": 1000,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(1, 0, 0)


def test_MessageToAWSEmptyMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 60,
        "str": "test",
        "val": 1000,
        "macs": []
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(1, 0, 0)
