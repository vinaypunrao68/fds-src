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
import os
import time


# This class contains the attributes and methods to test
# the transaction load provided by the com.formationds.smoketest.SmokeTestRunner suite.
class TestSmokeLoad(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestSmokeLoad, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_SmokeLoad():
                test_passed = False
        except Exception as inst:
            self.log.error("Smoke load caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_SmokeLoad(self):
        """
        Test Case:
        Attempt to run the the com.formationds.smoketest.SmokeTestRunner suite.

        WARNING: This implementation assumes a localhost node in all cases.
        """

        # Currently, 10/13/2014, we require a bit of a delay after all components are
        # up so that they can initialize themselves among each other before proceeding,
        # confident that we have a workable system.
        delay = 20
        self.log.warning("Waiting an obligatory %d seconds to allow FDS components to initialize among themselves before "
                         "starting test load." %
                         delay)
        time.sleep(delay)

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_get_obj('cfg_am')
        for n in nodes:
            self.log.info("Apply smoke load to node %s." % n.nd_conf_dict['fds_node'])

            cur_dir = os.getcwd()
            os.chdir(fdscfg.rt_env.env_fdsSrc + '/Build/linux-x86_64.debug/tools')
            status, stdout = n.nd_am_node.nd_agent.exec_wait('./smokeTest %s' %
                                                     n.nd_am_node.nd_host, return_stdin=True)
            if stdout is not None:
                self.log.info(stdout)

            os.chdir(cur_dir)

            if status != 0:
                self.log.error("Application of smoke load on %s returned status %d." %(n.nd_conf_dict['fds_node'], status))
                return False

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()