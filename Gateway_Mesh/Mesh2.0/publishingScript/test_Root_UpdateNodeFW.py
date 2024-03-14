import main
import globalVars


# Standard test commented out, it will reset the nodes and disrupt testing
# def test_NodeUpdateFW():
#     globalVars.resetVars()
# data = {
#     "usrID": "testID",
#     "cmnd": 61,
#     "val": main.numOfNodes,
#     "macs": main.macs,
#     "str": "https://spacrfirmware.s3.ca-central-1.amazonaws.com/Mesh2_0.bin"
# }

#     sendToRoot(data)
#     main.sendToRoot(data)
#     globalVars.timeout_assert(1, 0, 0)


# Test incorrect/missing val numbers
def test_UpdateNodeFWMissingVal():
    globalVars.resetVars()

    data = {
        "usrID": "testID",
        "cmnd": 61,
        "macs": main.macs,
        # "val": main.numOfNodes,
        "str": main.OTA_NODE_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWValMoreThanMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes + 1,
        "macs": main.macs,
        "str": main.OTA_NODE_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWMacsMoreThanVal():
    globalVars.resetVars()

    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes - 1,
        "macs": main.macs,
        "str": main.OTA_NODE_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWValZero():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": 0,
        "macs": main.macs,
        "str": main.OTA_NODE_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


# Testing incorrect/missing macs


def test_UpdateNodeFWMissingMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        "str": main.OTA_NODE_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWCorruptedMacs():
    globalVars.resetVars()
    macCopy = main.macs.copy()
    macCopy[3] = None
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        "macs": macCopy,
        "str": main.OTA_NODE_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWExtendedMacs():
    globalVars.resetVars()
    macCopy = main.macs.copy()
    macCopy.append(0xaa)
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        "macs": macCopy,
        "str": main.OTA_NODE_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


# Testing incorrect/missing URLs


def test_UpdateNodeFWMissingURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        # "str": main.OTA_URL,
        "macs": main.macs
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)

# Test missing/corrupted URL


def test_UpdateNodeFWMissingURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        "macs": main.macs
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWCorruptURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        "macs": main.macs,
        "str": "12345" + main.OTA_NODE_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWEmptyMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": 0,
        "macs": [],
        "str": main.OTA_NODE_URL + "12345"
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateNodeFWWrongURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 61,
        "val": main.numOfNodes,
        "macs": main.macs,
        "str": main.OTA_NODE_URL + "12345"
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
