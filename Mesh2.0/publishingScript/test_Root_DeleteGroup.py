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
    time.sleep(5)


def test_DeleteGroup(initialize_group):
    globalVars.resetVars()
    print("Standard command")
    data = {
        "usrID": "testID",
        "cmnd": 63,
        "str": main.grpID,
        "val": main.numOfNodes,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, main.numOfNodes + 1, 0)


def test_DeleteGroupNoMacs(initialize_group):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 63,
        "str": main.grpID,
        "val": main.numOfNodes
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_DeleteGroupMissingAttrs(initialize_group):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 63,
        "str": main.grpID,
        # "val": main.numOfNodes,
        "macs": main.macs
    }
    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_DeleteGroupEmptyMacs(initialize_group):
    globalVars.resetVars()
    corruptedMacs = main.macs.copy()
    corruptedMacs[2] = None
    print("corrupted macs")
    data = {
        "usrID": "testID",
        "cmnd": 63,
        "str": main.grpID,
        "val": main.numOfNodes,
        "macs": []
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)


def test_DeleteGroupCorruptedMacs(initialize_group):
    globalVars.resetVars()
    corruptedMacs = main.macs.copy()
    corruptedMacs[2] = None
    print("corrupted macs")
    data = {
        "usrID": "testID",
        "cmnd": 63,
        "str": main.grpID,
        "val": main.numOfNodes,
        "macs": corruptedMacs
    }

    main.sendToRoot(data)
    globalVars.timeout_assert(0, 0, 1)
