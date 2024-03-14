import main
import globalVars
import pytest
import time


@pytest.fixture
def overrideNodeType():
    # try to override node type before removing
    data = {
        "usrID": "testID",
        "cmnd": 10,
        "str": "",
        "val": 2
    }
    main.sendToNode(data)
    # above teardown triggers success messages, wait for it to arrive and clear
    time.sleep(5)
    globalVars.resetVars()
    return
    # teardown code to run


def test_RemoveNodeTypeOverride(overrideNodeType):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 11,
        "str": "",
        "val": 2
    }
    main.sendToNode(data)
    globalVars.timeout_assert(2, 0, 0)


def test_RemoveNodeTypeOverrideMissingAttr(overrideNodeType):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 11,
        # "str": "",
        "val": 2
    }
    main.sendToNode(data)
    globalVars.timeout_assert(0, 0, 1)


def test_RemoveNodeTypeOverrideExtraAttr(overrideNodeType):
    globalVars.resetVars()
    data = {
        "usrID": "testID",
        "cmnd": 11,
        "str": "",
        "val": 2,
        "macs": main.macs
    }
    main.sendToNode(data)
    globalVars.timeout_assert(2, 0, 0)
