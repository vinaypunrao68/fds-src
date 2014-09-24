#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
from subprocess import check_call
import TestCase

# Module-specific requirements
import sys
import time


# This class contains the attributes and methods to test
# activation of an FDS node where PM and OM have been started.
class TestNodeActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestNodeActivate, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_NodeActivate():
                test_passed = False
        except Exception as inst:
            self.log.error("Node activation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_NodeActivate(self):
        """
        Test Case:
        Attempt to activate a node where OM and PM have been started.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()

            self.log.info("Activate node %s." % n.nd_conf_dict['node-name'])

            status = n.nd_rmt_agent.ssh_exec_wait("cd %s; ./fdscli --fds-root %s --activate-nodes abc -k 1 -e am,dm,sm > %s/cli.out 2>&1" %
                                                  (bin_dir, fds_dir, log_dir))

            if status != 0:
                self.log.error("Node activation on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

            time.sleep(2)

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()