import main
import globalVars


def test_OverrideNodeType():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 10,
        "str": "",
        "val": 2
    }
    main.sendToNode(data)
    globalVars.timeout_assert(2, 0, 0)


def test_OverrideNodeTypeMissingAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 10,
        # "str": "",
        "val": 2
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_OverrideNodeTypeExtraAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 10,
        "str": "",
        "val": 2,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(2, 0, 0)


def test_OverrideNodeTypeInvalidVal():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 10,
        "str": "",
        "val": 500
    }
    main.sendToNode(data)
    globalVars.timeout_assert(2, 0, 0)
