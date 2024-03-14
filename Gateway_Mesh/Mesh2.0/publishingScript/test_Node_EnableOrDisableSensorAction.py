import main
import globalVars


def test_EnableOrDisableSA():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 6,
        "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_EnableOrDisableSAMissingAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 6,
        # "str": main.sensorMac,
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_EnableOrDisableSAInvalidMAC():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 6,
        "str": main.sensorMac[:-1],
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_EnableOrDisableSAExtraAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 6,
        "str": main.sensorMac,
        "val": 0,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)
