import main
import globalVars


def test_AddOrgInfo():
    globalVars.resetVars()
    print("Normal AddOrgInfo")
    data = {
        "usrID": "testID",
        "cmnd": 55,
        "str": main.orgID,
        "val": 0
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 1, 0)


def test_AddOrgInfoMissingStr():
    globalVars.resetVars()
    print("Missing str")
    data = {

        "usrID": "testID",
        "cmnd": 55,
        # "str": main.orgID,
        "val": 0
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
