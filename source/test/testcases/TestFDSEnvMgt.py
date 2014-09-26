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


# This class contains attributes and methods to test
# creating an FDS installation from a development environment.
class TestFDSCreateInstDir(TestCase.FDSTestCase):
    def __init__(self, parameters = None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)


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
            if not self.test_FDSCreateInstDir():
                test_passed = False
        except Exception as inst:
            self.log.error("FDS installation directory creation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSCreateInstDir(self):
        """
        Test Case:
        Attempt to create the FDS installation directory structure
        for an installation based upon a development environment.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            if not n.nd_rmt_agent.env_install:
                fds_dir = n.nd_conf_dict['fds_root']

                # Check to see if the FDS root directory is already there.
                status = n.nd_rmt_agent.ssh_exec_wait('ls ' + fds_dir)
                if status != 0:
                    # Not there, try to create it.
                    self.log.info("FDS installation directory, %s, nonexistent on node %s. Attempting to create." %
                                  (fds_dir, n.nd_conf_dict['node-name']))
                    status = n.nd_rmt_agent.ssh_exec_wait('mkdir -p ' + fds_dir)
                    if status != 0:
                        self.log.error("FDS installation directory creation on node %s returned status %d." %
                                       (n.nd_conf_dict['node-name'], status))
                        return False
                else:
                    self.log.warn("FDS installation directory, %s, exists on node %s." %
                                  (fds_dir, n.nd_conf_dict['node-name']))

                # Populate application configuration directory if necessary.
                # It should be necessary since we did not do a package install.
                dest_config_dir = fds_dir + "/etc"
                # Check to see if the etc directory is already there.
                status = n.nd_rmt_agent.ssh_exec_wait('ls ' + dest_config_dir)
                if status != 0:
                    # Not there, try to create it.
                    self.log.info("FDS configuration directory, %s, nonexistent on node %s. Attempting to create." %
                                  (dest_config_dir, n.nd_conf_dict['node-name']))
                    status = n.nd_rmt_agent.ssh_exec_wait('mkdir -p ' + dest_config_dir)
                    if status != 0:
                        self.log.error("FDS configuration directory creation on node %s returned status %d." %
                                       (n.nd_conf_dict['node-name'], status))
                        return False
                else:
                    self.log.warn("FDS configuration directory, %s, exists on node %s." %
                                  (dest_config_dir, n.nd_conf_dict['node-name']))

                # Load the product configuration directory from the development environment.
                src_config_dir = fdscfg.rt_env.get_config_dir()
                status = n.nd_rmt_agent.ssh_exec_wait('cp -rf ' + src_config_dir + '/* ' + dest_config_dir)
                if status != 0:
                    self.log.error("FDS configuration directory population on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                self.log.error("Test method %s is meant to be called when testing "
                               "from a development environment. Instead, try class TestFDSPkgInst." %
                               sys._getframe().f_code.co_name)
                return False

        return True


# This class contains attributes and methods to test
# creating an FDS installation from an FDS installation package.
class TestFDSPkgInst(TestCase.FDSTestCase):
    def __init__(self, parameters = None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)


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
            if not self.test_FDSPkgInst():
                test_passed = False
        except Exception as inst:
            self.log.error("FDS package installation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSPkgInst(self):
        """
        Test Case:
        Attempt to install the FDS package onto the specified nodes.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            status = n.nd_install_rmt_pkg()

            if status != 0:
                self.log.error("FDS package installation for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

        return True


# This class contains attributes and methods to test
# deleting an FDS installation.
class TestFDSDeleteInstDir(TestCase.FDSTestCase):
    def __init__(self, parameters = None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters)


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
            if not self.test_FDSDeleteInstDir():
                test_passed = False
        except Exception as inst:
            self.log.error("FDS installation directory deletion caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_FDSDeleteInstDir(self):
        """
        Test Case:
        Attempt to delete the FDS installation directory structure.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']

            # Check to see if the FDS root directory is already there.
            status = n.nd_rmt_agent.ssh_exec_wait('ls ' + fds_dir)
            if status == 0:
                # Try to delete it.
                self.log.info("FDS installation directory, %s, exists on node %s. Attempting to delete." %
                              (fds_dir, n.nd_conf_dict['node-name']))
                print fds_dir
                status = n.nd_cleanup_node()
                if status != 0:
                    self.log.error("FDS installation directory deletion on node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                self.log.warn("FDS installation directory, %s, nonexistent on node %s." %
                              (fds_dir, n.nd_conf_dict['node-name']))

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()