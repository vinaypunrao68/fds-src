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


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()