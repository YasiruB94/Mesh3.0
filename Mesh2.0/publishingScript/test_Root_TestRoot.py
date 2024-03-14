import main
import globalVars


def test_TestRoot():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 51,
        "val": 0,
        "str": ""
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(1, 1, 0)


def test_TestRootExtraMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 51,
        "val": 0,
        "str": "",
        "macs": main.macs
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(1, 1, 0)
