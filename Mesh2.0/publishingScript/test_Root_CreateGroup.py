import main
import globalVars
import time


def test_CreateGroup():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes,
        "str": main.grpID,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, main.numOfNodes + 1, 0)


def test_CreategroupMissingAttr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes,
        # "str": main.grpID,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_CreateGroupMacValMismatch():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes + 1,
        "str": main.grpID,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_CreateGroupShortMac():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes,
        "str": main.grpID,
        "macs": main.macs[:-1]
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_CreategroupEmptyMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes,
        "str": main.grpID,
        "macs": []
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_CreateGroupCorruptedMac():
    globalVars.resetVars()
    macsCopy = main.macs.copy()

    macsCopy[3] = None
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes,
        "str": main.grpID,
        "macs": macsCopy
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
