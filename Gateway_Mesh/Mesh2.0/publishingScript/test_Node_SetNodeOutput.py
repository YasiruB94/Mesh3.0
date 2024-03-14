import main
import globalVars


def test_SetNodeOutput():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 2,
        "str": "",
        "val": 1
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)


def test_SetNodeOutputNegValue():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 2,
        "str": "",
        "val": -1
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 1, 0)
