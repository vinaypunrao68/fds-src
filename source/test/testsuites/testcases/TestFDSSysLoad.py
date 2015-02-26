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


# This class contains the attributes and methods to test
# the transaction load provided by the com.formationds.smoketest.SmokeTestRunner suite.
class TestSmokeLoad(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SmokeLoad,
                                             "Smoke load")

    def test_SmokeLoad(self):
        """
        Test Case:
        Attempt to run the the com.formationds.smoketest.SmokeTestRunner suite.

        WARNING: This implementation assumes a localhost node in all cases.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            self.log.info("Apply smoke load to node %s." % n.nd_conf_dict['node-name'])

            cur_dir = os.getcwd()
            os.chdir(fdscfg.rt_env.env_fdsSrc + '/Build/linux-x86_64.debug/tools')
            status, stdout = n.nd_agent.exec_wait('./smokeTest %s' %
                                                     n.nd_host, return_stdin=True)
            if stdout is not None:
                self.log.info(stdout)

            os.chdir(cur_dir)

            if status != 0:
                self.log.error("Application of smoke load on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()