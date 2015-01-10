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
import os
import logging


def fileSearch(searchFile, searchString, occurrences):
    """
    Search the given file for the given search string
    verifying whether it occurs the specified number
    of times.
    """
    log = logging.getLogger('TestFDSSysMgt' + '.' + 'fileSearch')

    # Verify the file exists.
    if not os.path.isfile(searchFile):
        log.error("File %s is not found." % (searchFile))
        return False

    f = open(searchFile, "r")
    searchLines = f.readlines()
    f.close()

    occur = 0
    log.info("Searching for exactly %d occurrence(s) of string [%s]." % (occurrences, searchString))
    for line in searchLines:
        if searchString in line:
            occur = occur + 1
            log.info("Found occurrence %d: %s." % (occur, line))

    if occur == occurrences:
        return True
    else:
        return False


# This class contains the attributes and methods to test
# activation of an FDS cluster.
class TestClusterActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestClusterActivate, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_ClusterActivate():
                test_passed = False
        except Exception as inst:
            self.log.error("Cluster activation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_ClusterActivate(self):
        """
        Test Case:
        Attempt to activate a cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        n = fdscfg.rt_om_node
        fds_dir = n.nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        self.log.info("Activate cluster from OM node %s." % n.nd_conf_dict['node-name'])

        cur_dir = os.getcwd()
        os.chdir(bin_dir)

        status = n.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s --activate-nodes abc -k 1 -e dm,sm > '
                                      '%s/cli.out 2>&1 &) \"' %
                                      (fds_dir, log_dir if n.nd_agent.env_install else "."))

        os.chdir(cur_dir)

        if status != 0:
            self.log.error("Cluster activation on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to give
# a node some time to initialize. Once proper boot-up
# coordination has been implemented, this can be removed.
class TestWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestWait, self).__init__(parameters)


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
        Give a node some time to initialize.
        """

        # Currently, 10/13/2014, we require a bit of a delay after all components are
        # up so that they can initialize themselves among each other before proceeding,
        # confident that we have a workable system.
        delay = 20
        self.log.warning("Waiting an obligatory %d seconds to allow FDS components to initialize among themselves before "
                         "starting test load." %
                         delay)
        time.sleep(delay)

        return True


# This class contains the attributes and methods to test
# the shutdown of an FDS node.
class TestNodeShutdown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestNodeShutdown, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        # We'll continue to shutdown the nodes even in the event of failure upstream.
        if TestCase.pyUnitTCFailure and not True:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_NodeShutdown():
                test_passed = False
        except Exception as inst:
            self.log.error("Node shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_NodeShutdown(self):
        """
        Test Case:
        Attempt to shutdown all FDS daemons on a node.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()

            self.log.info("Shutdown node %s." % n.nd_conf_dict['node-name'])

            # TODO(GREG): Currently, no return from this method.
            n.nd_cleanup_daemons()

            #if status != 0:
            #    self.log.error("Node shutdown on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            #    return False

        return True


# This class contains the attributes and methods to test
# service removal of transient nodes.
class TestTransientRemoveService(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestTransientRemoveService, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_TransientRemoveService():
                test_passed = False
        except Exception as inst:
            self.log.error("Transient node remove service caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_TransientRemoveService(self):
        """
        Test Case:
        Attempt to remove the services of transient nodes.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # Skip the PM for the OM's node.
            if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                self.log.info("Skipping OM's PM on %s." % n.nd_conf_dict['node-name'])
                continue

            # Skip the PM for non-transient nodes. Those are handled by TestPMWait()
            if not n.nd_transient:
                self.log.info("Skipping PM on non-transient node %s." % n.nd_conf_dict['node-name'])
                continue

            self.log.info("Remove services for transient node %s." % n.nd_conf_dict['node-name'])

            cur_dir = os.getcwd()
            os.chdir(bin_dir)

            status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s --remove-services %s > '
                                          '%s/cli.out 2>&1 &) \"' %
                                          (fds_dir, n.nd_conf_dict['node-name'],
                                           log_dir if n.nd_agent.env_install else "."))

            os.chdir(cur_dir)

            if status != 0:
                self.log.error("Service removal of transient node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

        return True


# This class contains the attributes and methods to test
# whether there exists the expected initial DMT migration
# completion OM log entries for a 1-node cluster.
class TestInitial1NodeDMTMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestInitial1NodeDMTMigration, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_Initial1NodeDMTMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("Initial 1-node DMT migration caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_Initial1NodeDMTMigration(self):
        """
        Test Case:
        Attempt locate in OM's log logged statements indicating
        successful completion of the initial DMT migration for
        a 1-node cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_file = fds_dir + "/var/logs/om.log_0.log"

        if fileSearch(log_file, "OM_PmAgent: will send node activate message to Node-1", 1) and\
                fileSearch(log_file, "Sent DMT to 1 DM services successfully", 1) and\
                fileSearch(log_file, "Sent DMT to 1 AM services successfully", 1) and\
                fileSearch(log_file, "Sent DMT to 1 SM services successfully", 1) and\
                fileSearch(log_file, "OM deployed DMT with 1 DMs", 1):
            return True
        else:
            self.log.error("OM log entries confirming DMT migration for initial 1-node cluster not found.")
            return False


# This class contains the attributes and methods to test
# whether there exists the expected DMT migration
# completion OM log entries for a 1-node cluster with a node added.
class TestTransientAddNodeDMTMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestTransientAddNodeDMTMigration, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_TransientAddNodeDMTMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("Transient add node DMT migration caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_TransientAddNodeDMTMigration(self):
        """
        Test Case:
        Attempt locate in OM's log logged statements indicating
        successful completion of the DMT migration following the
        addition of a second node to a 1-node cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_file = fds_dir + "/var/logs/om.log_0.log"

        # Some of these message will have been logged before for other reasons, so account for them.
        if fileSearch(log_file, "OM_PmAgent: will send node activate message to Node-1", 2) and\
                fileSearch(log_file, "OM_PmAgent: will send node activate message to Node-2", 1) and\
                fileSearch(log_file, "Sent DMT to 2 DM services successfully", 1) and\
                fileSearch(log_file, "Sent DMT to 1 AM services successfully", 2) and\
                fileSearch(log_file, "Sent DMT to 2 SM services successfully", 1) and\
                fileSearch(log_file, "OM deployed DMT with 2 DMs", 1):
            return True
        else:
            self.log.error("OM log entries confirming DMT migration for adding a second node to an initial 1-node cluster not found.")
            return False


# This class contains the attributes and methods to test
# whether there exists the expected DMT migration
# completion OM log entries for a 2-node cluster following a node removed.
class TestTransientRemoveNodeDMTMigration(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(TestTransientRemoveNodeDMTMigration, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_TransientRemoveNodeDMTMigration():
                test_passed = False
        except Exception as inst:
            self.log.error("Transient remove node DMT migration caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_TransientRemoveNodeDMTMigration(self):
        """
        Test Case:
        Attempt locate in OM's log logged statements indicating
        successful completion of the DMT migration following the
        removal of a second node from a 2-node cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_file = fds_dir + "/var/logs/om.log_0.log"

        # Some of these message will have been logged before for other reasons, so account for them.
        if fileSearch(log_file, "Received remove services for nodeNode-2 remove am ? true remove sm ? true remove dm ? true", 1) and\
                fileSearch(log_file, "Sent DMT to 1 DM services successfully", 2) and\
                fileSearch(log_file, "Sent DMT to 1 AM services successfully", 3) and\
                fileSearch(log_file, "Sent DMT to 1 SM services successfully", 2) and\
                fileSearch(log_file, "OM deployed DMT with 1 DMs", 2):
            return True
        else:
            self.log.error("OM log entries confirming DMT migration for removing a node from a 2-node cluster not found.")
            return False


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()