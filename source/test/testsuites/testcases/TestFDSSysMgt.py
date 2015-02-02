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
from TestFDSModMgt import TestAMShutDown, TestSMShutDown, TestDMShutDown, TestOMShutDown, TestPMShutDown


# This class contains the attributes and methods to test
# activation of an FDS cluster except for AMs. For AMs, see
# test case TestFDSModMgt.TestAMActivate().
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

        self.log.info("Activate cluster (except AMs) from OM node %s." % n.nd_conf_dict['node-name'])

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
    def __init__(self, parameters=None, node=None):
        super(TestNodeShutdown, self).__init__(parameters)

        self.passedNode = node


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
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through the defined nodes shutting them down.
            if self.passedNode is not None:
                shutdownSuccess = False
                n = self.passedNode

            self.log.info("Shutdown node %s." % n.nd_conf_dict['node-name'])

            # First shutdown AM if on this node.
            amNodes = fdscfg.rt_get_obj('cfg_am')
            for amNode in amNodes:
                if amNode.nd_conf_dict['fds_node'] == n.nd_conf_dict['node-name']:
                    shutdownAM = TestAMShutDown(node=amNode)
                    shutdownSuccess = shutdownAM.test_AMShutDown()

                    if not shutdownSuccess:
                        self.log.error("Node shutdown on %s failed." % (n.nd_conf_dict['node-name']))
                        return False

                    break

            # SM and DM next.
            shutdownSM = TestSMShutDown(node=n)
            shutdownSuccess = shutdownSM.test_SMShutDown()

            if not shutdownSuccess:
                self.log.error("Node shutdown on %s failed." % (n.nd_conf_dict['node-name']))
                return False

            shutdownDM = TestDMShutDown(node=n)
            shutdownSuccess = shutdownDM.test_DMShutDown()

            if not shutdownSuccess:
                self.log.error("Node shutdown on %s failed." % (n.nd_conf_dict['node-name']))
                return False

            # Next, shutdown OM if on this node.
            if fdscfg.rt_om_node.nd_conf_dict['node-name'] == n.nd_conf_dict['node-name']:
                shutdownOM = TestOMShutDown(node=n)
                shutdownSuccess = shutdownOM.test_OMShutDown()

                if not shutdownSuccess:
                    self.log.error("Node shutdown on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            # Finally PM.
            shutdownPM = TestPMShutDown(node=n)
            shutdownSuccess = shutdownPM.test_PMShutDown()

            if not shutdownSuccess:
                self.log.error("Node shutdown on %s failed." % (n.nd_conf_dict['node-name']))
                return False

            if self.passedNode is not None:
                # We took care of the specified node. Now get out.
                break

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
# services removal of nodes. (I.e. remove the node from
# the cluster.)
class TestNodeRemoveServices(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(TestNodeRemoveServices, self).__init__(parameters)

        self.passedNode = node


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_NodeRemoveService():
                test_passed = False
        except Exception as inst:
            self.log.error("Node services removal caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_NodeRemoveService(self):
        """
        Test Case:
        Attempt to remove the services of nodes from a cluster.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were provided a node, remove that one and exit.
            if self.passedNode is not None:
                n = self.passedNode
            else:
                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                    self.log.info("Skipping OM's PM on %s." % n.nd_conf_dict['node-name'])
                    continue

                # Skip the PM for non-transient nodes.
                if not n.nd_transient:
                    self.log.info("Skipping PM on non-transient node %s." % n.nd_conf_dict['node-name'])
                    continue

            self.log.info("Remove services for node %s." % n.nd_conf_dict['node-name'])

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
            elif self.passedNode is not None:
                # If we were passed a specific node, exit now.
                return True

        return True


# This class contains the attributes and methods to test
# domain shutdown
class TestDomainShutdown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_DomainShutdown():
                test_passed = False
        except Exception as inst:
            self.log.error("Domain shutdown caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_DomainShutdown(self):
        """
        Test Case:
        Attempt to execute domain-shutdown.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']

        self.log.info("Shutdown domain.")

        cur_dir = os.getcwd()
        os.chdir(bin_dir)

        status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s --domain-shutdown abc -k 1 > '
                                      '%s/cli.out 2>&1 &) \"' %
                                      (fds_dir,
                                       log_dir if om_node.nd_agent.env_install else "."))

        os.chdir(cur_dir)

        if status != 0:
            self.log.error("Domain shutdown returned status %d." % (status))
            return False
        else:
            return True


# This class contains the attributes and methods to test
# domain create
class TestDomainCreate(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters)


    def runTest(self):
        test_passed = True

        if TestCase.pyUnitTCFailure:
            self.log.warning("Skipping Case %s. stop-on-fail/failfast set and a previous test case has failed." %
                             self.__class__.__name__)
            return unittest.skip("stop-on-fail/failfast set and a previous test case has failed.")
        else:
            self.log.info("Running Case %s." % self.__class__.__name__)

        try:
            if not self.test_DomainCreate():
                test_passed = False
        except Exception as inst:
            self.log.error("Domain create caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_DomainCreate(self):
        """
        Test Case:
        Attempt to execute domain-create.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']

        self.log.info("Create domain.")

        cur_dir = os.getcwd()
        os.chdir(bin_dir)

        status = om_node.nd_agent.exec_wait('bash -c \"(nohup ./fdscli --fds-root %s --domain-create abc -k 1 > '
                                      '%s/cli.out 2>&1 &) \"' %
                                      (fds_dir,
                                       log_dir if om_node.nd_agent.env_install else "."))

        os.chdir(cur_dir)

        if status != 0:
            self.log.error("Domain create returned status %d." % (status))
            return False
        else:
            return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()