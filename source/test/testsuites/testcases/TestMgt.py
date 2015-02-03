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
        super(TestWait, self).__init__(parameters)

        if delay is not None:
            if isinstance(delay, int):
                self.passedDelay = delay
            else:
                self.passedDelay = int(delay)
        else:
            self.passedDelay = delay

        self.passedReason = reason


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_Wait():
                test_passed = False
        except Exception as inst:
            self.log.error("Wait caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


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