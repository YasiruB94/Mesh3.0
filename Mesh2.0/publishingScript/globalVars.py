import time

logsCounter = 0
logsMsgs = []
controlDataSuccess = 0
successMsgs = []
controlDataFail = 0
failMsgs = []
timeout = 10


def resetVars():
    global logsCounter, controlDataSuccess, controlDataFail
    logsCounter = 0
    controlDataSuccess = 0
    controlDataFail = 0
    logsMsgs.clear()
    successMsgs.clear()
    failMsgs.clear()


def timeout_assert(goalLogsCounter, goalControlSuccess, goalControlFail):
    timeout_start = time.time()
    while time.time() < timeout_start + timeout:
        if(logsCounter == goalLogsCounter
                and controlDataSuccess == goalControlSuccess
                and controlDataFail == goalControlFail):
            break
    # check for extra messages
    time.sleep(1)
    assert logsCounter == goalLogsCounter
    assert controlDataSuccess == goalControlSuccess
    assert controlDataFail == goalControlFail

def timeout_assert_type_02(timeout_in_seconds, goalLogsCounter, goalControlSuccess, goalControlFail):
    timeout_start = time.time()
    while time.time() < timeout_start + timeout:
        if(logsCounter == goalLogsCounter
                and controlDataSuccess == goalControlSuccess
                and controlDataFail == goalControlFail):
            break
    # check for extra messages
    time.sleep(timeout_in_seconds)
    assert logsCounter == goalLogsCounter
    assert controlDataSuccess == goalControlSuccess
    assert controlDataFail == goalControlFail