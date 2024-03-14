import main
import globalVars


# Standard test commented out, it will reset the nodes and disrupt testing
# def test_UpdateSensorFW():
#     globalVars.resetVars()
# data = {
#     "usrID": "testID",
#     "cmnd": 66,
#     "val": main.numOfNodes,
#     "macs": main.macs,
#     "str": OTA_SENSOR_URL
# }

#     sendToRoot(data)
#     main.sendToRoot(data)
#     globalVars.timeout_assert(1, 0, 0)


# Test incorrect/missing val numbers
def test_UpdateSensorFWMissingVal():
    globalVars.resetVars()

    data = {
        "usrID": "testID",
        "cmnd": 66,
        "macs": main.macs,
        # "val": main.numOfNodes,
        "str": main.OTA_SENSOR_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWValMoreThanMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes + 1,
        "macs": main.macs,
        "str": main.OTA_SENSOR_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWMacsMoreThanVal():
    globalVars.resetVars()

    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes - 1,
        "macs": main.macs,
        "str": main.OTA_SENSOR_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWValZero():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": 0,
        "macs": main.macs,
        "str": main.OTA_SENSOR_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


# Testing incorrect/missing macs


def test_UpdateSensorFWMissingMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        "str": main.OTA_SENSOR_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWCorruptedMacs():
    globalVars.resetVars()
    macCopy = main.macs.copy()
    macCopy[3] = None
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        "macs": macCopy,
        "str": main.OTA_SENSOR_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWExtendedMacs():
    globalVars.resetVars()
    macCopy = main.macs.copy()
    macCopy.append(0xaa)
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        "macs": macCopy,
        "str": main.OTA_SENSOR_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


# Testing incorrect/missing URLs


def test_UpdateSensorFWMissingURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        # "str": main.OTA_SENSOR_URL,
        "macs": main.macs
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)

# Test missing/corrupted URL


def test_UpdateSensorFWMissingURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        "macs": main.macs
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWCorruptURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        "macs": main.macs,
        "str": "12345" + main.OTA_SENSOR_URL
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWWrongURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": main.numOfNodes,
        "macs": main.macs,
        "str": main.OTA_SENSOR_URL + "12345"
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateSensorFWEmptyMacs():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 66,
        "val": 0,
        "macs": [],
        "str": main.OTA_SENSOR_URL + "12345"
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
