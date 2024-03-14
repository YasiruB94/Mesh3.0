import globalVars
import main
import pytest
import time


@pytest.fixture
def initialize_group():
    data = {
        "usrID": "testID",
        "cmnd": 62,
        "val": main.numOfNodes,
        "str": main.grpID,
        "macs": main.macs
    }
    main.sendToRoot(data)
    # above teardown triggers success messages, wait for it to arrive and clear
    time.sleep(10)
    globalVars.resetVars()


def test_SendGroupCmd(initialize_group):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 54,
        "str": f'{{"grpID": "{main.grpID}", "usrID": "testID", "cmnd": 1, "str": "", "val": 0}}',
        "val": 0
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(main.numOfNodes, main.numOfNodes + 1, 0)


def test_SendGroupCmdCorruptedMacs(initialize_group):
    globalVars.resetVars()
    corruptedMacs = main.macs.copy()
    corruptedMacs[3] = None
    data = {
        "usrID": "testID",
        "cmnd": 54,
        "str": f'{{"grpID": "{main.grpID}", "usrID": "testID", "cmnd": 1, "str": "", "val": 0}}',
        "val": 6,
        "macs": corruptedMacs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(main.numOfNodes, main.numOfNodes + 1, 0)


def test_SendGroupCmdInvalidID(initialize_group):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 54,
        "str": '{"grpID": "200", "usrID": "testID", "cmnd": 1, "str": "", "val": 0}',
        "val": 0
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
    print("invalid grpID")


def test_SendGroupCmdNonExistID(initialize_group):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 54,
        "str": '{"grpID": "10", "usrID": "testID", "cmnd": 1, "str": "", "val": 0}',
        "val": 0
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 1, 0)
