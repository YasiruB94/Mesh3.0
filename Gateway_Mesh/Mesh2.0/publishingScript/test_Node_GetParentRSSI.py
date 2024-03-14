import main
import globalVars


def test_GetParentRSSI():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 15,
        "str": "",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(1, 1, 0)


def test_GetParentRSSIMissingATTR():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 15,
        # "str": "",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)
