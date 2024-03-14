import main
import globalVars
import json


def test_GetNodeChildren():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 16,
        "str": "",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(1, 1, 0)
    if(globalVars.logsCounter == 1):
        resultJSON = json.loads(globalVars.logsMsgs[0])
        assert ("childNodes" in resultJSON)


def test_GetNodeChildrenMissingAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 16,
        # "str": "",
        "val": 0
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)
