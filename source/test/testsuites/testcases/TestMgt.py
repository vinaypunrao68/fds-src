#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase

# Module-specific requirements
import sys
import time


# This class contains the attributes and methods to give
# a node some time to initialize. Once proper boot-up
# coordination has been implemented, this can be removed.
class TestWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, delay=None, reason=None):
        super(TestWait, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_Wait,
                                             "Wait")

        if delay is not None:
            if isinstance(delay, int):
                self.passedDelay = delay
            else:
                self.passedDelay = int(delay)
        else:
            self.passedDelay = delay

        self.passedReason = reason

    def test_Wait(self):
        """
        Test Case:
        Cause the test run to sleep for a specified number of seconds.
        """

        if self.passedDelay is not None:
            delay = self.passedDelay
        else:
            delay = 20

        if self.passedReason is not None:
            reason = self.passedReason
        else:
            reason = "just because"

        self.log.info("Waiting %d seconds %s." % (delay, reason))
        time.sleep(delay)

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()