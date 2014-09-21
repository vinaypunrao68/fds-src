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


# This class contains the attributes and methods to test
# bringing up a Data Manager (DM) module.
class TestDMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestDMBringUp, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_DMBringUp():
                test_passed = False
        except Exception as inst:
            self.log.error("DM module bringup caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_DMBringUp(self):
        """
        Test Case:
        Attempt to start the DM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(False)  # No debugging.

            self.log.info("Start DM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_rmt_agent.ssh_exec_wait("%s/DataMgr --fds-root=%s > %s/dm.out" % (bin_dir, fds_dir, bin_dir))

            if status != 0:
                self.log.error("DM on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# bringing up a Storage Manager (SM) module.
class TestSMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(TestSMBringUp, self).__init__(parameters)


    def runTest(self):
        """
        Used by qaautotest module's test runner to run the test case
        and clean up the fixture as necessary.

        With PyUnit, the same method is run although PyUnit will also
        call any defined tearDown method to do test fixture cleanup as well.
        """
        test_passed = True

        self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_SMBringUp():
                test_passed = False
        except Exception as inst:
            self.log.error("SM module bring caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_SMBringUp(self):
        """
        Test Case:
        Attempt to start the SM module(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fds_dir + '/bin'

            self.log.info("Start SM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_rmt_agent.ssh_exec_wait("%s/StorMgr --fds-root=%s > %s/sm.out" % (bin_dir, fds_dir, bin_dir))

            if status != 0:
                self.log.error("SM on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()