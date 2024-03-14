import main
import globalVars


def test_ChangePortPIR():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 9,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_ChangePortPIRMissingAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 9,
        # "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_ChangePortPIRExtraAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 9,
        "str": main.sensorMac,
        "val": 0,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_ChangePortPIRInvalidMac():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 9,
        "str": main.sensorMac[:-1],
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_ChangePortPIRInvalidVal():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 9,
        "str": main.sensorMac,
        "val": -12
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)
