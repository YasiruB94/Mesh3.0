import main
import globalVars


def test_TestNode():
    globalVars.resetVars()
    print("normal testNode")
    data = {
        "usrID": "testID",
        "cmnd": 1,
        "str": "hello",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(1, 1, 0)


def test_TestNodeMissingAttr():
    globalVars.resetVars()
    print("normal testNode")
    data = {
        "usrID": "testID",
        "cmnd": 1,
        # "str": "hello",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)
