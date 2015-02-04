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
from TestFDSModMgt import TestAMKill, TestSMKill, TestDMKill, TestOMKill, TestPMKill
from TestFDSModMgt import TestAMBringup


# This class contains the attributes and methods to test
# activation of an FDS cluster starting the same, specified
# services on each node.
class TestClusterActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, services="dm,sm,am"):
        super(TestClusterActivate, self).__init__(parameters)

        self.passedServices = services


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

        self.log.info("Activate cluster starting %s services on each node." % self.passedServices)

        cur_dir = os.getcwd()
        os.chdir(bin_dir)

        status = n.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s --activate-nodes abc -k 1 -e %s > '
                                      '%s/cli.out 2>&1) \"' %
                                      (fds_dir, self.passedServices, log_dir if n.nd_agent.env_install else "."))

        os.chdir(cur_dir)

        if status != 0:
            self.log.error("Cluster activation on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        # After activation we should be able to spin through our nodes to obtain
        # some useful information about them.
        for n in fdscfg.rt_obj.cfg_nodes:
            status = n.nd_populate_metadata(_bin_dir=bin_dir)
            if status != 0:
                break

        if status != 0:
            self.log.error("Node meta-data lookup for %s returned status %d." % (n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# the kill the services of an FDS node.
class TestNodeKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(TestNodeKill, self).__init__(parameters)

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
            if not self.test_NodeKill():
                test_passed = False
        except Exception as inst:
            self.log.error("Node kill caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_NodeKill(self):
        """
        Test Case:
        Attempt to kill all services on a node.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through the defined nodes killing them.
            if self.passedNode is not None:
                n = self.passedNode

            self.log.info("Kill node %s." % n.nd_conf_dict['node-name'])

            # First kill AM if on this node.
            if n.nd_services.count("am") > 0:
                killAM = TestAMKill(node=n)
                killSuccess = killAM.test_AMKill()

                if not killSuccess:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            # SM and DM next.
            if n.nd_services.count("sm") > 0:
                killSM = TestSMKill(node=n)
                killSuccess = killSM.test_SMKill()

                if not killSuccess:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            if n.nd_services.count("dm") > 0:
                killDM = TestDMKill(node=n)
                killSuccess = killDM.test_DMKill()

                if not killSuccess:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            # Next, kill OM if on this node.
            if fdscfg.rt_om_node.nd_conf_dict['node-name'] == n.nd_conf_dict['node-name']:
                killOM = TestOMKill(node=n)
                killSuccess = killOM.test_OMKill()

                if not killSuccess:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            # Finally PM.
            killPM = TestPMKill(node=n)
            killSuccess = killPM.test_PMKill()

            if not killSuccess:
                self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                return False

            if self.passedNode is not None:
                # We took care of the specified node. Now get out.
                break

        return True


# This class contains the attributes and methods to test
# node activation. (I.e. activate the node's specified
# services.)
class TestNodeActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters)

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
            if not self.test_NodeActivateService():
                test_passed = False
        except Exception as inst:
            self.log.error("Node services activation caused exception:")
            self.log.error(traceback.format_exc())
            test_passed = False

        super(self.__class__, self).reportTestCaseResult(test_passed)

        # If there is any test fixture teardown to be done, do it here.

        if self.parameters["pyUnit"]:
            self.assertTrue(test_passed)
        else:
            return test_passed


    def test_NodeActivateService(self):
        """
        Test Case:
        Attempt to activate the services of the specified node(s).
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        nodes = fdscfg.rt_obj.cfg_nodes

        for n in nodes:
            # If we were provided a node, activate that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            # If we're asked to activate nodes, we should be able to spin through our nodes to obtain
            # some useful information about them.
            status = n.nd_populate_metadata(_bin_dir=bin_dir)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            # By handling AM separately in a multi-node environment,
            # we avoid FS-879.
            if len(nodes) > 1:
                services = n.nd_services.replace('am', '')
            else:
                services = n.nd_services

            if services != '':
                self.log.info("Activate services %s for node %s." % (services, n.nd_conf_dict['node-name']))

                cur_dir = os.getcwd()
                os.chdir(bin_dir)

                status = om_node.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s --activate-services abc -k 1 -w %s -e %s > '
                                              '%s/cli.out 2>&1) \"' %
                                              (fds_dir, int(n.nd_uuid, 16), services,
                                               log_dir if n.nd_agent.env_install else "."))

                os.chdir(cur_dir)

            # By handling AM separately in a multi-node environment,
            # we avoid FS-879.
            if (n.nd_services.count('am') > 0) and (len(nodes) > 1):
                # Boot instead of activate.
                amBootTest = TestAMBringup(node=n)
                amBooted = amBootTest.test_AMBringUp()
                if not amBooted:
                    self.log.error("AM failed to boot on node %s." % n.nd_conf_dict['node-name'])
                    return False

            if status != 0:
                self.log.error("Service activation of node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            elif self.passedNode is not None:
                # If we were passed a specific node, exit now.
                return True

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
#            else: Should we skip the OM's node?
#                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
#                    self.log.info("Skipping OM's node on %s." % n.nd_conf_dict['node-name'])
#                    continue

            self.log.info("Remove services for node %s." % n.nd_conf_dict['node-name'])

            cur_dir = os.getcwd()
            os.chdir(bin_dir)

            status = om_node.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s --remove-services %s -e %s > '
                                          '%s/cli.out 2>&1) \"' %
                                          (fds_dir, n.nd_assigned_name, n.nd_services,
                                           log_dir if n.nd_agent.env_install else "."))

            os.chdir(cur_dir)

            if status != 0:
                self.log.error("Service removal of node %s returned status %d." %
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

        status = om_node.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s --domain-shutdown abc -k 1 > '
                                      '%s/cli.out 2>&1) \"' %
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

        status = om_node.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s --domain-create abc -k 1 > '
                                      '%s/cli.out 2>&1) \"' %
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