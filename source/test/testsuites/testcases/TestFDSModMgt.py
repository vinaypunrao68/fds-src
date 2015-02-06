#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback

import xmlrunner
import TestCase


# Module-specific requirements
import sys
import time
import logging
import os
import shlex


def getSvcPIDforNode(svc, node, javaClass=None):
    """
    Return the process ID for the given
    service/node. Returns -1 if not found.

    Note: When searching for a java process, set svc to
    "java" and javaClass to the class name used to start the application.
    """
    log = logging.getLogger('TestFDSModMgt' + '.' + 'getSvcPIDforNode')

    status = 0
    cmd = "pgrep %s" % svc

    status, stdout = node.nd_agent.exec_wait(cmd, return_stdin=True)

    if len(stdout) > 0:
        # We've got a list of process IDs for the service of interest.
        # We need to make sure that among them is the one for the node of interest.
        for pid in shlex.split(stdout):
            cmd2 = "ps -www --no-headers %s" % pid
            status, stdout2 = node.nd_agent.exec_wait(cmd2, return_stdin=True)

            if len(stdout2) > 0:
                # We've got the details of the process. Let's see if it is supporting
                # the node of interest.
                cmd3 = "grep %s" % node.nd_conf_dict['node-name']
                status, stdout3 = node.nd_agent.exec_wait(cmd3, return_stdin=True, cmd_input=stdout2)

                # For java processes, we have also to look for the initial class of execution.
                if (len(stdout3) > 0) and (svc == "java") and (javaClass is not None):
                    cmd3_1 = "grep %s" % javaClass
                    status, stdout3 = node.nd_agent.exec_wait(cmd3_1, return_stdin=True, cmd_input=stdout3)
                elif (svc == "java") and (javaClass is None):
                    log.error("When searching for a java process also include the java class name.")
                    raise Exception

                if len(stdout3) > 0:
                    # Found it!
                    return pid

    # Didn't find it.
    return -1


def pidWaitParent(pid, child_count, node):
    """
    Wait for indicated process to present a specified number of children.
    """
    log = logging.getLogger('TestFDSModMgt' + '.' + 'pidWaitParent')

    maxcount = 20

    count = 0
    found = False
    status = 0
    cmd = "pgrep -c -P %s" % pid
    while (count < maxcount) and not found:
        time.sleep(1)

        status, stdout = node.nd_agent.exec_wait(cmd, return_stdin=True)

        if int(stdout) == child_count:
            found = True

        count += 1

    if status != 0:
        log.error("Wait for %d children of %s on %s returned status %d." %
                  (child_count, pid, node.nd_conf_dict['node-name'], status))
        return False

    if not found:
        return False

    return True

def modWait(mod, node, forShutdown = False):
    """
    Wait for the named service to become active.
    """
    log = logging.getLogger('TestFDSModMgt' + '.' + 'modWait')

    maxcount = 20

    orchMgrPID = ''
    AMAgentPID = ''
    count = 0

    # When we don't want to see the process, assume
    # that it's there, otherwise, assume that it's
    # not there.
    if forShutdown:
        found = True
    else:
        found = False

    status = 0
    cmd = "pgrep %s" % mod
    while (count < maxcount) and ((forShutdown and found) or (not forShutdown and not found)):
        time.sleep(1)

        pid = getSvcPIDforNode(mod, node)
        if pid != -1:
            found = True

            if not forShutdown:
                # For process orchMgr, there should be one child process.
                if mod == "orchMgr":
                    orchMgrPID = pid

                # For process AMAgent, there should be two child processes.
                if mod == "AMAgent":
                    AMAgentPID = pid
        else:
            found = False

        count += 1

    if status != 0:
        if not forShutdown:
            log.error("Wait for %s on %s returned status %d." % (mod, node.nd_conf_dict['node-name'], status))
            return False

    if not found:
        if forShutdown:
            return True
        else:
            return False
    else:
        if forShutdown:
            return False

    if orchMgrPID != '':
        return pidWaitParent(int(orchMgrPID), 1, node)

    if AMAgentPID != '':
        return pidWaitParent(int(AMAgentPID), 2, node)

    return True


# This class contains the attributes and methods to test
# bringing up a Data Manager (DM) service.
class TestDMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMBringUp,
                                             "DM service bringup")

        self.passedNode = node

    def test_DMBringUp(self):
        """
        Test Case:
        Attempt to start the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we boot all DMs we have.
            if self.passedNode is not None:
                n = self.passedNode

            port = n.nd_conf_dict['fds_port']
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()

            # Make sure DM should be running on this node.
            if n.nd_services.count("dm") == 0:
                self.log.warning("DM not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is None:
                    break
                else:
                    continue

            self.log.info("Start DM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait('bash -c \"(nohup %s/DataMgr --fds-root=%s > %s/dm.%s.out 2>&1 &) \"' %
                                          (bin_dir, fds_dir, log_dir, port))

            if status != 0:
                self.log.error("DM bringup on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # Passed a specific node so get out.
                break

            time.sleep(2)

        return True


# This class contains the attributes and methods to test
# waiting for a Data Manager (DM) service to start.
class TestDMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMWait,
                                             "Wait for DM service to start")

        self.passedNode = node

    def test_DMWait(self):
        """
        Test Case:
        Wait for the DM service(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, check that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            # Make sure this node is configured for DM.
            if "dm" not in n.nd_services:
                self.log.warning("DM service not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is not None:
                    break
                else:
                    continue

            self.log.info("Wait for DM on %s." % n.nd_conf_dict['node-name'])

            if not modWait("DataMgr", n):
                return False

            if self.passedNode is not None:
                break

        return True


# This class contains the attributes and methods to test
# killing a Data Manager (DM) service.
class TestDMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMKill,
                                             "DM service kill")

        self.passedNode = node

    def test_DMKill(self):
        """
        Test Case:
        Attempt to shutdown the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, shutdown that one and get out.
            if self.passedNode is not None:
                n = self.passedNode

            # Make sure DM should be running on this node.
            if n.nd_services.count("dm") == 0:
                self.log.warning("DM not configured to run on node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Kill DM on %s." % n.nd_conf_dict['node-name'])
            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('DataMgr', n)
            if pid != -1:
                cmd = "kill -9 %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if status != 0:
                    self.log.error("DM kill on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("DM not running on %s." % (n.nd_conf_dict['node-name']))

            if status != 0:
                self.log.error("DM kill on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # Took care of the node passed in so get out.
                break

        return True


# This class contains the attributes and methods to test
# whether a Data Manager (DM) service is down.
class TestDMVerifyDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMVerifyDown,
                                             "Verify a DM service is down")

        self.passedNode = node

    def test_DMVerifyDown(self):
        """
        Test Case:
        Wait for the DM service(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, shutdown that one and get out.
            if self.passedNode is not None:
                n = self.passedNode

            self.log.info("Verify the DM on %s is down." %n.nd_conf_dict['node-name'])

            if not modWait("DataMgr", n, forShutdown=True):
                return False
            elif self.passedNode is not None:
                # Took care of the node passed in so get out.
                break

        return True


# This class contains the attributes and methods to test
# bringing up a Storage Manager (SM) service.
class TestSMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SMBringUp,
                                             "SM service bringup")

        self.passedNode = node

    def test_SMBringUp(self):
        """
        Test Case:
        Attempt to start the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we boot all SMs we have.
            if self.passedNode is not None:
                n = self.passedNode

            port = n.nd_conf_dict['fds_port']
            fds_dir = n.nd_conf_dict['fds_root']
            bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
            log_dir = fdscfg.rt_env.get_log_dir()

            # Make sure SM should be running on this node.
            if n.nd_services.count("sm") == 0:
                self.log.warning("SM not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is None:
                    break
                else:
                    continue

            self.log.info("Start SM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait('bash -c \"(nohup %s/StorMgr --fds-root=%s > %s/sm.%s.out 2>&1 &) \"' %
                                          (bin_dir, fds_dir, log_dir, port))

            if status != 0:
                self.log.error("SM on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # Passed a specific node so get out.
                break

        return True


# This class contains the attributes and methods to test
# waiting for a Storage Manager (SM) service to start.
class TestSMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SMWait,
                                             "Wait for SM service to start")

        self.passedNode = node

    def test_SMWait(self):
        """
        Test Case:
        Wait for the SM service(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, check that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            # Make sure this node is configured for SM.
            if "sm" not in n.nd_services:
                self.log.warning("SM service not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is not None:
                    break
                else:
                    continue


            self.log.info("Wait for SM on %s." %n.nd_conf_dict['node-name'])

            if not modWait("StorMgr", n):
                return False

            if self.passedNode is not None:
                break

        return True


# This class contains the attributes and methods to test
# killing a Storage Manager (SM) service.
class TestSMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SMKill,
                                             "SM service kill")

        self.passedNode = node

    def test_SMKill(self):
        """
        Test Case:
        Attempt to shutdown the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, shutdown that one and get out.
            if self.passedNode is not None:
                n = self.passedNode

            # Make sure SM should be running on this node.
            if n.nd_services.count("sm") == 0:
                self.log.warning("SM not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is None:
                    break
                else:
                    continue

            self.log.info("Kill SM on %s." %n.nd_conf_dict['node-name'])
            # Get the PID of the process in question and ... kill it!
            pid = getSvcPIDforNode('StorMgr', n)
            if pid != -1:
                cmd = "kill -9 %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if status != 0:
                    self.log.error("SM kill on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("SM not running on %s." % (n.nd_conf_dict['node-name']))

            if status != 0:
                self.log.error("SM kill on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # Took care of the one node passed so get out.
                break

        return True


# This class contains the attributes and methods to test
# whether a Storage Manager (SM) service is down.
class TestSMVerifyDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SMVerifyDown,
                                             "Verify the SM service is down")

        self.passedNode = node

    def test_SMVerifyDown(self):
        """
        Test Case:
        Wait for the SM service(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, shutdown that one and get out.
            if self.passedNode is not None:
                n = self.passedNode

            self.log.info("Verify the SM on %s is down." %n.nd_conf_dict['node-name'])

            if not modWait("StorMgr", n, forShutdown=True):
                return False
            elif self.passedNode is not None:
                # Took care of the node passed in so get out.
                break

        return True


# This class contains the attributes and methods to test
# bringing up a Platform Manager (PM) component.
class TestPMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMBringUp,
                                             "PM service bringup")

        self.passedNode = node

    def test_PMBringUp(self):
        """
        Test Case:
        Attempt to start the PM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through in defined node setting it up.
            if self.passedNode is not None:
                n = self.passedNode
            else:
                # Skip the PM for the OM's node. That one is handled by TestPMForOMBringUp()
                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                    self.log.info("Skipping OM's PM on %s." %n.nd_conf_dict['node-name'])
                    continue

            self.log.info("Start PM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_start_platform(om_ip, test_harness=True, _bin_dir=bin_dir, _log_dir=log_dir)

            if status != 0:
                self.log.error("PM on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # If we were passed a specific node, just install
                # that one and exit.
                return True

        return True


# This class contains the attributes and methods to test
# waiting for a Platform Manager (PM) service to start.
class TestPMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMWait,
                                             "Wait for PM service to start")

        self.passedNode = node

    def test_PMWait(self):
        """
        Test Case:
        Wait for the PM service(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through all defined nodes setting them up.
            if self.passedNode is not None:
                n = self.passedNode
            else:
                # Skip the PM for the OM's node. That one is handled by TestPMForOMWait()
                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                    self.log.info("Skipping OM's PM on %s." %n.nd_conf_dict['node-name'])
                    continue

            self.log.info("Wait for PM on %s." %n.nd_conf_dict['node-name'])

            if not modWait("platformd", n):
                return False
            elif self.passedNode is not None:
                # If we were passed a specific node, just take care
                # of that one and exit.
                return True

        return True


# This class contains the attributes and methods to test
# killing a Platform Manager (PM) service.
class TestPMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMKill,
                                             "PM service kill")

        self.passedNode = node

    def test_PMKill(self):
        """
        Test Case:
        Attempt to shutdown the PM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, shutdown that one and get out.
            if self.passedNode is not None:
                n = self.passedNode
            else:
                # Skip the PM for the OM's node. That one is handled by TestPMForOMKill()
                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                    self.log.info("Skipping OM's PM on %s." %n.nd_conf_dict['node-name'])
                    continue

            self.log.info("Kill PM on %s." %n.nd_conf_dict['node-name'])
            # Get the PID of the process in question and ... kill it!
            pid = getSvcPIDforNode('platformd', n)
            if pid != -1:
                cmd = "kill -9 %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if status != 0:
                    self.log.error("PM kill on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("PM is not running on %s." % (n.nd_conf_dict['node-name']))

            if status != 0:
                self.log.error("PM kill on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # Took care of the node passed in. Get out.
                break

        return True


# This class contains the attributes and methods to test
# whether a Platform Manager (PM) service is down .
class TestPMVerifyDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMVerifyDown,
                                             "Verify a PM service is down")

        self.passedNode = node

    def test_PMVerifyDown(self):
        """
        Test Case:
        Verify the PM service(s) are down
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, shutdown that one and get out.
            if self.passedNode is not None:
                n = self.passedNode
            else:
                # Skip the PM for the OM's node. That one is handled by TestPMForOMKill()
                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                    self.log.info("Skipping OM's PM on %s." % n.nd_conf_dict['node-name'])
                    continue

            self.log.info("Verify the PM on %s is down." % n.nd_conf_dict['node-name'])

            if not modWait("platformd", n, forShutdown=True):
                return False
            elif self.passedNode is not None:
                # Took care of the node passed in. Get out.
                break

        return True


# This class contains the attributes and methods to test
# bringing up a Platform Manager (PM) for the Orchestration Manager (OM) node.
class TestPMForOMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMForOMBringUp,
                                             "PM for OM service bringup")

    def test_PMForOMBringUp(self):
        """
        Test Case:
        Attempt to start the PM component for an OM node
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()
        om_node = fdscfg.rt_om_node

        # Start the PM for this OM.
        om_ip = om_node.nd_conf_dict['ip']

        self.log.info("Start PM for OM on node %s." % om_node.nd_conf_dict['node-name'])

        status = om_node.nd_start_platform(om_ip, test_harness=True, _bin_dir=bin_dir, _log_dir=log_dir)

        if status != 0:
            self.log.error("PM for OM on node %s returned status %d." % (om_node.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# waiting for the Platform Manager (PM) service for the OM node to start.
class TestPMForOMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMForOMWait,
                                             "Wait for PM service on OM node to start")

        self.passedNode = node

    def test_PMForOMWait(self):
        """
        Test Case:
        Wait for the PM service on the OM node to start
        """
        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If a list of nodes were passed, make sure it includes
        # the OM's node. If not, do not check this PM.
        if self.passedNode is not None:
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = fdscfg.rt_om_node
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Wait for PM on OM's node, %s." % om_node.nd_conf_dict['node-name'])

            if not modWait("platformd", om_node):
                return False
        else:
            self.log.warn("PM on non-OM node, %s, not to be checked." % self.passedNode.nd_conf_dict['node-name'])

        return True


# This class contains the attributes and methods to test
# killing the Platform Manager (PM) service on the OM node.
class TestPMForOMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMForOMKill,
                                             "PM service for OM node kill")

    def test_PMForOMKill(self):
        """
        Test Case:
        Attempt to shutdown the PM service for the OM node
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        self.log.info("Kill PM on OM's node, %s." % om_node.nd_conf_dict['node-name'])

        status = om_node.nd_agent.exec_wait("pkill -9 platformd")

        if status != 0:
            self.log.error("PM kill on OM's node, %s, returned status %d." %
                           (om_node.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# whether the Platform Manager (PM) service for the OM's node is down.
class TestPMForOMVerifyDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_PMForOMVerifyDown,
                                             "Verifying the PM service for the OM's node is down")

        self.passedNode = node

    def test_PMForOMVerifyDown(self):
        """
        Test Case:
        Verify the PM service for the OM's node is down
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node and it is not the OM node
        # captured during config parsing, just exit.
        if self.passedNode is not None:
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Verify the PM on OM's node, %s, is down." % om_node.nd_conf_dict['node-name'])

            if not modWait("platformd", om_node, forShutdown=True):
                return False

        return True


# This class contains the attributes and methods to test
# bringing up an Orchestration Manager (OM) service.
class TestOMBringUp(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_OMBringUp,
                                             "OM service bringup")

        self.passedNode = node

    def test_OMBringUp(self):
        """
        Test Case:
        Attempt to start the OM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        # If we were passed a node and it is not the OM node
        # captured during config parsing, just exit.
        if self.passedNode is not None:
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Start OM on %s." % om_node.nd_conf_dict['node-name'])

            status = om_node.nd_start_om(test_harness=True, _bin_dir=bin_dir, _log_dir=log_dir)

            if status != 0:
                self.log.error("OM on %s returned status %d." % (om_node.nd_conf_dict['node-name'], status))
                return False

        return True


# This class contains the attributes and methods to wait
# for the Orchestration Manager (OM) service to become active.
class TestOMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_OMWait,
                                             "OM service wait")

        self.passedNode = node

    def test_OMWait(self):
        """
        Test Case:
        Wait for the named service to become active.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node and it is not the OM node
        # captured during config parsing, just exit.
        if self.passedNode is not None:
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Wait for OM on %s." % om_node.nd_conf_dict['node-name'])

            return modWait("orchMgr", om_node)
        else:
            return True


# This class contains the attributes and methods to test
# killing an Orchestration Manager (OM) service.
class TestOMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_OMKill,
                                             "OM service kill")

        self.passedNode = node

    def test_OMKill(self):
        """
        Test Case:
        Attempt to shutdown the OM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]


        # If we were passed a node and it is not the OM node
        # captured during config parsing, just exit.
        if self.passedNode is not None:
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Kill OM on %s." % om_node.nd_conf_dict['node-name'])

            status = om_node.nd_agent.exec_wait('pkill -9 -f com.formationds.om.Main')

            # Probably we get the -9 return because pkill for a java process will
            # not find it based on the class name alone. See getSvcPIDforNode().
            if status != -9:
                self.log.error("OM (com.formationds.om.Main) kill on %s returned status %d." %
                               (om_node.nd_conf_dict['node-name'], status))
                return False

            status = om_node.nd_agent.exec_wait("pkill -9 orchMgr")

            if (status != 1) and (status != 0):
                self.log.error("OM (orchMgr) kill on %s returned status %d." %
                               (om_node.nd_conf_dict['node-name'], status))
                return False

        return True


# This class contains the attributes and methods to
# verify that the the Orchestration Manager (OM) service is down.
class TestOMVerifyDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_OMVerifyDown,
                                             "OM service verify down")

        self.passedNode = node

    def test_OMVerifyDown(self):
        """
        Test Case:
        Verify the OM is down.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        # If we were passed a node and it is not the OM node
        # captured during config parsing, just exit.
        if self.passedNode is not None:
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Verify the OM on %s is down." % fdscfg.rt_om_node.nd_conf_dict['node-name'])

            return modWait("orchMgr", fdscfg.rt_om_node, forShutdown=True)

        return True


# This class contains the attributes and methods to test
# bringing up an Access Manager (AM) service.
class TestAMBringup(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMBringUp,
                                             "AM service bringup")

        self.passedNode = node

    def test_AMBringUp(self):
        """
        Test Case:
        Attempt to start the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        nodes = fdscfg.rt_obj.cfg_nodes

        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we boot all AMs we have.
            if self.passedNode is not None:
                n = self.passedNode

            # Make sure AM should be running on this node.
            if n.nd_services.count("am") == 0:
                self.log.warning("AM not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is None:
                    break
                else:
                    continue

            self.log.info("Start AM on %s." % n.nd_conf_dict['node-name'])

            port = n.nd_conf_dict['fds_port']
            fds_dir = n.nd_conf_dict['fds_root']

            # The AMAgent script expected to be invoked from the bin directory in which it resides.
            cur_dir = os.getcwd()
            os.chdir(bin_dir)
            status = n.nd_agent.exec_wait('bash -c \"(nohup ./AMAgent --fds-root=%s -fds.am.instanceId=%s 0<&- &> ./am.%s.out &) \"' %
                                                     (fds_dir, n.nd_nodeID, port))
            os.chdir(cur_dir)

            if status != 0:
                self.log.error("AM on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # We were passed a specific AM so get out now.
                return True

        return True


# This class contains the attributes and methods to test
# activating Access Manager (AM) services on a cluster.
class TestAMActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMActivate,
                                             "AM service activation")

    def test_AMActivate(self):
        """
        Test Case:
        Attempt to activate the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        n = fdscfg.rt_om_node
        fds_dir = n.nd_conf_dict['fds_root']
        bin_dir = fdscfg.rt_env.get_bin_dir(debug=False)
        log_dir = fdscfg.rt_env.get_log_dir()

        self.log.info("Activate cluster AMs from OM node %s." % n.nd_conf_dict['node-name'])

        cur_dir = os.getcwd()
        os.chdir(bin_dir)

        status = n.nd_agent.exec_wait('bash -c \"(./fdscli --fds-root %s --activate-nodes abc -k 1 -e am > '
                                      '%s/cli.out 2>&1) \"' %
                                      (fds_dir, log_dir if n.nd_agent.env_install else "."))

        os.chdir(cur_dir)

        if status != 0:
            self.log.error("AM activation on %s returned status %d." %(n.nd_conf_dict['node-name'], status))
            return False

        return True


# This class contains the attributes and methods to test
# waiting for an Access Manager (AM) service to start.
class TestAMWait(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMWait,
                                             "Wait for AM service to start")

        self.passedNode = node

    def test_AMWait(self):
        """
        Test Case:
        Wait for the AM service(s) to start
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, check that one and exit.
            if self.passedNode is not None:
                # But only check it if it should have an AM.
                if self.passedNode.nd_conf_dict['node-name'] != n.nd_conf_dict['node-name']:
                    continue

            # Make sure this node is configured for AM.
            if n.nd_services.count('am') == 0:
                self.log.warning("AM service not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is not None:
                    break
                else:
                    continue

            self.log.info("Wait for AM on %s." % n.nd_conf_dict['node-name'])

            if not modWait("AMAgent", n):
                return False

            if self.passedNode is not None:
                break

        return True


# This class contains the attributes and methods to test
# killing an Access Manager (AM) service.
class TestAMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMKill,
                                             "AM service kill")

        self.passedNode = node

    def test_AMKill(self):
        """
        Test Case:
        Attempt to shutdown the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = self.passedNode

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("am") == 0:
                self.log.warning("AM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Shutdown AM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('java', n, javaClass='com.formationds.am.Main')
            if pid != -1:
                cmd = "kill -9 %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if status != 0:
                    self.log.error("AM (com.formationds.am.Main) shutdown on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("AM (com.formationds.am.Main) already shutdown on %s." % (n.nd_conf_dict['node-name']))

            pid = getSvcPIDforNode('bare_am', n)
            if pid != -1:
                cmd = "kill -9 %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if status != 0:
                    self.log.error("AM (bare_am) shutdown on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("AM (bare_am) already shutdown on %s." % (n.nd_conf_dict['node-name']))

            pid = getSvcPIDforNode('AMAgent', n)
            if pid != -1:
                cmd = "kill -9 %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if (status != 1) and (status != 0):
                    self.log.error("AM (AMAgent) shutdown on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("AM (AMAgent) already shutdown on %s." % (n.nd_conf_dict['node-name']))

            if (status != 1) and (status != 0):
                self.log.error("AM shutdown on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# whether an Access Manager (AM) service is down.
class TestAMVerifyDown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMVerifyDown,
                                             "Verify the AM service is down")

        self.passedNode = node

    def test_AMVerifyDown(self):
        """
        Test Case:
        Verify the AM service(s) are down
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a specific node, check that one and get out.
            if self.passedNode is not None:
                n = self.passedNode

            self.log.info("Verify AM on %s is down" % n.nd_conf_dict['node-name'])

            if not modWait("AMAgent", n, forShutdown=True):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))