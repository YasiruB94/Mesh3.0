import main
import globalVars


# working test commented out, will restart root and disrupt tests
# def test_UpdateRootFW():
#     globalVars.resetVars()
#     data = {
#         "usrID": "testID",
#         "cmnd": 52,
#         "val": 0,
#         "str": main.OTA_ROOT_URL
#     }
#     main.sendToRoot(data)
#     globalVars.timeout_assert(1, 0, 0)


def test_UpdaeRootFWMissingVal():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 52,
        # "val": 0,
        "str": main.OTA_ROOT_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdaeRootFWMissingStr():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 52,
        "val": 0,
        # "str": main.OTA_ROOT_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateRootFWCorruptURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 52,
        "val": 0,
        "str": "12345" + main.OTA_ROOT_URL
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateRootFWWrongURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 52,
        "val": 0,
        "str": main.OTA_ROOT_URL + "/12345"
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_UpdateRootFWNullURL():
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 52,
        "val": 0,
        "str": None
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
