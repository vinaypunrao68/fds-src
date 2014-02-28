#!/bin/bash
echo "================= Running Pre-Commit Tests ================="
#git stash -q --keep-index
pwd
cd source

test/fds-primitive-smoke.py
RESULT=$?
#git stash pop -q
if [ $RESULT -ne 0 ]; then
    echo "Pre-commit test Failed"
    exit 1
fi
echo "Pre-commit test Passed"
exit 0
