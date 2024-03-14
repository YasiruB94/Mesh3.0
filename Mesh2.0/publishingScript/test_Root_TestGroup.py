import globalVars
import main
import pytest
import time


@pytest.fixture
def initialize_group():
    time.sleep(10)
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


def test_TestGroup(initialize_group):
    data = {
        "usrID": "testID",
        "cmnd": 53,
        "str": main.grpID,
        "val": 0
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(main.numOfNodes, main.numOfNodes + 1, 0)


def test_TestGroupMissingAttr(initialize_group):
    data = {
        "usrID": "testID",
        "cmnd": 53,
        # "str": main.grpID,
        "val": 0
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_TestGroupValMacMismatch(initialize_group):
    data = {
        "usrID": "testID",
        "cmnd": 53,
        "str": main.grpID,
        "val": main.numOfNodes + 1,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(main.numOfNodes, main.numOfNodes + 1, 0)


def test_TestGroupEmptymacs(initialize_group):
    data = {
        "usrID": "testID",
        "cmnd": 53,
        "str": main.grpID,
        "val": 4,
        "macs": []
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(main.numOfNodes, main.numOfNodes + 1, 0)
