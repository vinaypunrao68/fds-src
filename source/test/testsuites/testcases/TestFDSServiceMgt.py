#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import pdb

import xmlrunner
import TestCase
from fdslib.TestUtils import findNodeFromInv
from fdslib import SvcHandle

# Module-specific requirements
import sys
import time
import logging
import shlex
import random
import re
import os
import fdslib.platformservice as plat_svc
import fdslib.FDSServiceUtils as FDSServiceUtils
from fdslib.TestUtils import get_node_service
from fdscli.model.fds_error import FdsError
from fdscli.model.platform.service import Service
sm_killed_pid = {}
dm_killed_pid = {}
bare_am_killed_pid = {}

def getSvcPIDforNode(svc, node, javaClass=None):
    """
    Return the process ID for the given
    service/node. Returns -1 if not found.

    Note: When searching for a java process, set svc to
    "java" and javaClass to the class name used to start the application.
    """
    log = logging.getLogger('TestFDSModMgt' + '.' + 'getSvcPIDforNode')

    cmd = "pgrep -f '(valgrind)?.*%s'" % svc

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
                if node.nd_agent.env_install:
                    # For java processes, we have also to look for the initial class of execution.
                    if (svc == "java") and (javaClass is not None):
                        if stdout2.count(javaClass) > 0:
                            return pid
                    elif (svc == "java") and (javaClass is None):
                        log.error("When searching for a java process also include the java class name.")
                        raise Exception
                    else:
                        if len(stdout.splitlines()) > 1:
                            # When the node is installed/deployed from a package,
                            # we expect only one process-set per service.
                            log.warning("More %s processes than expected: %s." % (svc, stdout))

                        return pid
                else:
                    # For java processes, we have also to look for the initial class of execution.
                    if (stdout2.count(node.nd_conf_dict['node-name']) > 0) and (svc == "java") and (javaClass is not None):
                        stdout2 = stdout2 if stdout2.count(javaClass) > 0 else ''
                    elif (svc == "java") and (javaClass is None):
                        log.error("When searching for a java process also include the java class name.")
                        raise Exception

                    if stdout2.count(node.nd_conf_dict['node-name']) > 0:
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
    cmd = "pgrep -c -P -f '(valgrind)?.*%s'" % pid
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
    Wait for the named service to become active or to stop according to forShutdown option.
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

    mod_decorator = mod
    java_class = None

    if mod == "orchMgr":
        mod_decorator = "java"
        java_class = "com.formationds.om.Main"

    if mod == "AMAgent":
        mod_decorator = "java"
        java_class = "com.formationds.am.Main"

    status = 0
    while (count < maxcount) and ((forShutdown and found) or (not forShutdown and not found)):
        time.sleep(1)

        pid = getSvcPIDforNode(mod_decorator, node, java_class)
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

#    if orchMgrPID != '':
#        return pidWaitParent(int(orchMgrPID), 1, node)
#
#    if AMAgentPID != '':
#        return pidWaitParent(int(AMAgentPID), 2, node)

    return True

def generic_kill(parameters, node, service):
    '''
    Generic method to kill a specified service running on a specified node.
    :param parameters: The caller's test fixture parameters.
    :param node: The node (object) to kill the service on
    :param service: The service to kill
    :return: An integer status code returned by the kill command
    '''

    status = -1
    log = logging.getLogger('TestFDSServiceMgt' + '.' + 'generic_kill')

    if service == "sm":
        killSM = TestSMKill(parameters=parameters, node=node)
        if killSM.test_SMKill(): status = 0
    elif service == "dm":
        killDM = TestDMKill(parameters=parameters, node=node)
        if killDM.test_DMKill(): status = 0
    elif service == "am":
        killAM = TestAMKill(parameters=parameters, node=node)
        if killAM.test_AMKill(): status = 0
    elif service == "om":
        killOM = TestOMKill(parameters=parameters, node=node)
        if killOM.test_OMKill(): status = 0
    elif service == "pm":
        killPM = TestPMKill(parameters=parameters, node=node)
        if killPM.test_PMKill(): status = 0
    else:
        log.error("Could not find service {} in service map. "
                  "Not sure what to kill, so aborting by returning -1!".format(service))

    return status

# This class contains the attributes and methods to test
# randomly killing a service on a random node after a
# random amount of time
# RANDOM!
class TestRndSvcKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, nodes=None,
                 time_window=None, services=None):
        '''
        :param parameters: Gets populated with .ini config params when run by qaautotest
        :param nodes: Nodes to randomly select from, separated by white space. If None all nodes considered.
        :param time_window: Dash separated integer pair (e.g. 0-120) representing min/max in seconds to
                            randomly select from. If None time will be now.
        :param services: Services to randomly select from, separated by white space. If None all services considered.
        '''

        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_RndSvcKill,
                                             "Random service killer")

        node_inv = self.parameters['fdscfg'].rt_obj.cfg_nodes
        # Listify!
        if nodes is not None:
            nodes = nodes.split(' ')
            # Also ensure that the nodes are proper objects
            # Hooray for partial application :)
            finder = lambda x: findNodeFromInv(node_inv, x)
            # Use finder to find each of the node objects
            self.passedNodes = map(finder, nodes)
        else:
            # If we weren't passed any nodes to pick from, pick from any of them
            self.passedNodes = node_inv

        if services is not None:
            self.passedSvcs = services.split(' ')
        else:
            self.passedSvcs = ['am', 'pm', 'dm', 'dm', 'om']

        if time_window is not None:
            self.passedWindow = tuple(map(lambda x: int(x), time_window.split('-')))
        else:
            self.passedWindow = (0, 0)

    def __pick_rnds(self):
        '''
        Picks random node/service pair and returns them.
        :return: (node, service) pair
        '''

        return (random.choice(self.passedNodes),
                random.choice(self.passedSvcs))

    def test_RndSvcKill(self):
        '''
        Will randomly kill a service within the bounds of the parameters passed to the constructor.
        This test assumes that if a node is up it's services are also up. If for some reason the node
        or services are already down it will return False.
        :return: True on successful kill, False otherwise
        '''

        # Pick our random node
        selected_node = random.choice(self.passedNodes)
        self.log.info("Selected {} as random node".format(selected_node.nd_conf_dict['node-name']))

        # Now select a random service
        selected_svc = random.choice(self.passedSvcs)
        self.log.info("Selected {} as random svc to kill".format(selected_svc))

        selected_time = random.choice(range(self.passedWindow[0], self.passedWindow[1]))
        self.log.info("Selected {} as random time after which to kill {} on node {}.".format(selected_time,
                                                                                             selected_svc,
                                                                                             selected_node.nd_conf_dict['node-name']))

        # Verify the service exists for the node, if not all bets are off, pick another random node/svc
        while(selected_node.nd_services.count(selected_svc) == 0 and selected_svc != 'pm'):
            self.log.warn("Service {} not configured for node {}."
                          "Selecting new random node/service".format(selected_svc,
                                                                     selected_node.nd_conf_dict['node-name']))
            selected_node = random.choice(self.passedNodes)
            selected_svc = random.choice(self.passedSvcs)

        # Add the node, service combo to params so we can track it
        if 'svc_killed' in self.parameters:
            self.parameters['svc_killed'].append((selected_node, selected_svc))
        else:
            self.parameters['svc_killed'] = [(selected_node, selected_svc)]

        # Try the kill
        # sleep first for the random wait aspect
        self.log.info("Sleeping {} seconds before kill.".format(selected_time))
        time.sleep(selected_time)
        status = generic_kill(self.parameters, selected_node, selected_svc)

        if status != 0:
            self.log.error("{} kill on {} returned status {}.".format(selected_svc,
                                                                      selected_node.nd_conf_dict['node-name'],
                                                                      status))
            return False

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
                n = findNodeFromInv(nodes, self.passedNode)

            port = n.nd_conf_dict['fds_port']
            fds_dir = n.nd_conf_dict['fds_root']
            log_dir = n.nd_agent.get_log_dir()

            # Make sure DM should be running on this node.
            if n.nd_services.count("dm") == 0:
                self.log.warning("DM not configured for node %s." % n.nd_conf_dict['node-name'])
                if self.passedNode is None:
                    break
                else:
                    continue

            self.log.info("Start DM on %s." %n.nd_conf_dict['node-name'])

            status = n.nd_agent.exec_wait('bash -c \"(nohup ./DataMgr --fds-root=%s > %s/dm.%s.out 2>&1 &) \"' %
                                          (fds_dir, log_dir, port),
                                          fds_bin=True)

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
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure it wasn't the target of a random kill
            if (n, "dm") in self.parameters.get('svc_killed', []):
                self.log.warning("DM service for node {} previous "
                                 "killed by random svc kill test.".format(n.nd_conf_dict['node-name']))
                if self.passedNode is not None:
                    break
                else:
                    continue

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
# activating Data Manager (DM) services on a domain.
class TestDMActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMActivate,
                                             "DM service activation")

        self.passedNode = node

    def test_DMActivate(self):
        """
        Test Case:
        Attempt to activate the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']
        node_service = get_node_service(self,om_ip)
        node_list = node_service.list_nodes()

        for each_node in node_list:

            # If we were provided a node, activate that one and exit.
            if self.passedNode is not None:
                self.log.info("Activate DM for node %s" % each_node.id)

                # Make sure this node is configured for an DM service.
                if self.passedNode.nd_services.count("dm") == 0:
                    self.log.error("Node %s is not configured to host a DM service." % self.passedNode.nd_conf_dict['node-name'])
                    return False

                # Make sure we have the node's meta-data.
                status = self.passedNode.nd_populate_metadata(om_node=om_node)
                if status != 0:
                    self.log.error("Getting meta-data for node %s returned status %d." %
                               (each_node.nd_conf_dict['node-name'], status))
                    return False

                node_id = int(self.passedNode.nd_uuid, 16)
                each_node = node_service.get_node(node_id)

            service = Service()
            service.type = 'DM'
            add_service = node_service.add_service(each_node.id,service)
            if type(add_service).__name__ == 'FdsError':
                self.log.error(" Add DM service failed on node %s"%(each_node.id))
                return False

            self.log.info("Activate service DM for node %s." % (each_node.id))
            start_service = node_service.start_service(each_node.id,add_service.id)
            time.sleep(3)
            get_service = node_service.get_service(each_node.id,start_service.id)
            if type(get_service).__name__ == 'FdsError' or get_service.status.state == "NOT_RUNNING":
                self.log.error("Service activation of node %s returned status %s." %
                        (each_node.id))
                return False

            if self.passedNode is not None:
                # If we were passed a specific node, exit now.
                break

        return True


# This class contains the attributes and methods to test
# stopping Data Manager (DM) services on a domain.
# We don't have test case that remove DM yet, as the need for it has not arisen.
class TestDMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DMStop,
                                             "DM service stopping")

        self.passedNode = node
    def test_DMStop(self):
        """
        Test Case:
        Attempt to stop the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']
        nodes = fdscfg.rt_obj.cfg_nodes
        for node in nodes:
            # If we were passed a node, check that one and exit.
            if self.passedNode is not None:
                node = self.passedNode

            # Make sure this node is configured for an DM service.
            if (node.nd_services.count("dm") == 0) and self.passedNode is not None:
                self.log.error("Node %s is not configured to host a DM service." % node.nd_conf_dict['node-name'])
                return False

            # Make sure we have the node's meta-data.
            status = node.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False

            self.log.info("Stopping DM for node %s using OM node %s." % (node.nd_conf_dict['node-name'],
                                                                        om_node.nd_conf_dict['node-name']))
            self.log.debug("DM's Node UUID should be: " + node.nd_uuid + " and in decimal: " + str(int(node.nd_uuid, 16)))
            node_service = get_node_service(self, om_ip)
            node_cli = node_service.get_node(int(node.nd_uuid, 16))
            try:
                dm_state = node_cli.services['DM'][0].status.state
                if dm_state == 'RUNNING':
                    #if dm is running then only stop it
                    dm_service = node_service.stop_service(node_cli.id,node_cli.services['DM'][0].id)
                    dm_state = dm_service.status.state
                assert dm_state == "NOT_RUNNING"
            except IndexError:
                self.log.error("Active DM service not found on %s ." % (node.nd_conf_dict['node-name']))
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
                n = findNodeFromInv(nodes, self.passedNode)

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
                cmd = "kill -KILL %s" % pid
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

            dm_killed_pid[n.nd_conf_dict['node-name']]= pid
            if self.passedNode is not None:
                # Took care of the node passed in so get out.
                break

        return True

# Deprecated TestAWSDMKill, we use TestDMKill for AWS nodes TODO: pooja remove class
# This class contains the attributes and methods to test
# killing an (DM) service.
class TestAWSDMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_DMKill,
                                             "DM service kill  ")

        self.passedNode = node

    def test_AWS_DMKill(self):
        """
        Test Case:
        Attempt to kill the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("dm") == 0:
                self.log.warning("DM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Killing DM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('DataMgr', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            dm_obj = FDSServiceUtils.DMService(om_ip, node_ip)
            ret_status = dm_obj.kill(node_ip)

            if ret_status:
                status = 0
                self.log.info("DM (DataMgr) has been killed and restarted on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to kill DM (DataMgr) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

# This class contains the attributes and methods to test
# killing an (DM) service.
class TestAWSDMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_DMStop,
                                             "Stop DM service  ")

        self.passedNode = node

    def test_AWS_DMStop(self):
        """
        Test Case:
        Attempt to stop the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)


            self.log.info("Stopping DM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('DataMgr', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            dm_obj = FDSServiceUtils.DMService(om_ip, node_ip)
            ret_status = dm_obj.stop(node_ip)

            if ret_status:
                status = 0
                self.log.info("DM (DataMgr) is no longer running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to stop DM (DataMgr) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# killing an (DM) service.
class TestAWSDMStart(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_DMStart,
                                             "Start DM service  ")

        self.passedNode = node

    def test_AWS_DMStart(self):
        """
        Test Case:
        Attempt to start the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Starting DM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('DataMgr', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            dm_obj = FDSServiceUtils.DMService(om_ip, node_ip)
            ret_status = dm_obj.start(node_ip)

            if ret_status:
                status = 0
                self.log.info("DM (DataMgr) is running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to start DM (DataMgr) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# adding Data Manager (DM) service.
class TestAWSDMAdd(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_AddDMService,
                                             "Add DM service  ")

        self.passedNode = node

    def test_AWS_AddDMService(self):
        """
        Test Case:
        Attempt to add the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Adding DM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            dm_obj = FDSServiceUtils.DMService(om_ip, node_ip)
            ret_status = dm_obj.add(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# removing Data Manager (DM) service.
class TestAWSDMRemove(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_RemoveDMService,
                                             "Remove DM service  ")

        self.passedNode = node

    def test_AWS_RemoveDMService(self):
        """
        Test Case:
        Attempt to remove the DM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Removing DM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            dm_obj = FDSServiceUtils.DMService(om_ip, node_ip)
            ret_status = dm_obj.remove(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
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
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Verify the DM on %s is down." %n.nd_conf_dict['node-name'])

            if not modWait("DataMgr", n, forShutdown=True):
                pid = getSvcPIDforNode('DataMgr', n)
                if pid != -1 and pid == dm_killed_pid[n.nd_conf_dict['node-name']] :
                    return False
                self.log.info("The DM on %s was down and was probably restarted by PM with new pid %s" %(n.nd_conf_dict['node-name'],pid))

            if self.passedNode is not None:
                # Took care of the node passed in so get out.
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
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure it wasn't the target of a random kill
            if (n, "sm") in self.parameters.get('svc_killed', []):
                self.log.warning("SM service for node {} previous "
                                 "killed by random svc kill test.".format(n.nd_conf_dict['node-name']))
                if self.passedNode is not None:
                    break
                else:
                    continue


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
# activating Storage Manager (SM) services on a domain.
class TestSMActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SMActivate,
                                             "SM service activation")

        self.passedNode = node

    def test_SMActivate(self):
        """
        Test Case:
        Attempt to activate the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']
        node_service = get_node_service(self,om_ip)
        node_list = node_service.list_nodes()

        for each_node in node_list:

            # Do all SMs in the domain or just the SM for the passed node?
            if self.passedNode is not None:
                self.log.info("Activate SM for node %s" % each_node.id)

                # Make sure this node is configured for an DM service.
                if self.passedNode.nd_services.count("sm") == 0:
                    self.log.error("Node %s is not configured to host a SM service." % self.passedNode.nd_conf_dict['node-name'])
                    return False

                # Make sure we have the node's meta-data.
                status = self.passedNode.nd_populate_metadata(om_node=om_node)
                if status != 0:
                    self.log.error("Getting meta-data for node %s returned status %d." %
                               (each_node.nd_conf_dict['node-name'], status))
                    return False

                node_id = int(self.passedNode.nd_uuid, 16)
                each_node = node_service.get_node(node_id)

            service = Service()
            service.type = 'SM'
            add_service = node_service.add_service(each_node.id,service)
            if type(add_service).__name__ == 'FdsError':
                self.log.error(" Add SM service failed on node %s"%(each_node.id))
                return False

            self.log.info("Activate service SM for node %s." % (each_node.id))
            start_service = node_service.start_service(each_node.id,add_service.id)
            time.sleep(3)
            get_service = node_service.get_service(each_node.id,start_service.id)
            if type(get_service).__name__ == 'FdsError' or get_service.status.state == "NOT_RUNNING":
                self.log.error("SM Service activation of node %s returned status %s." %
                        (each_node.id))
                return False

            if self.passedNode is not None:
                # If we were passed a specific node, exit now.
                break

        return True


# This class contains the attributes and methods to test
# stopping Storage Manager (SM) services on a domain.
# We don't have test cases that remove AM yet, as the need for it has not arisen.
class TestSMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_SMStop,
                                             "SM service Stopping")

        self.passedNode = node

    @TestCase.expectedFailure
    def test_SMStop(self):
        """
        Test Case:
        Attempt to stop the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']
        nodes = fdscfg.rt_obj.cfg_nodes
        for node in nodes:
            # If we were passed a node, check that one and exit.
            if self.passedNode is not None:
                node = self.passedNode

            # Make sure this node is configured for an SM service.
            if (node.nd_services.count("sm") == 0) and self.passedNode is not None:
                self.log.error("Node %s is not configured to host a SM service." % node.nd_conf_dict['node-name'])
                return False

            # Make sure we have the node's meta-data.
            status = node.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False

            self.log.info("Stopping SM for node %s using OM node %s." % (node.nd_conf_dict['node-name'],
                                                                        om_node.nd_conf_dict['node-name']))
            node_service = get_node_service(self, om_ip)
            node_cli = node_service.get_node(int(node.nd_uuid, 16))
            try:
                sm_state = node_cli.services['SM'][0].status.state
                if sm_state == 'RUNNING':
                    #if dm is running then only stop it
                    sm_service = node_service.stop_service(node_cli.id,node_cli.services['SM'][0].id)
                    sm_state = sm_service.status.state
                assert sm_state == "NOT_RUNNING"
            except IndexError:
                self.log.error("Active SM service not found on %s ." % (node.nd_conf_dict['node-name']))
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
                n = findNodeFromInv(nodes, self.passedNode)

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

            sm_killed_pid[n.nd_conf_dict['node-name']]= pid
            if self.passedNode is not None:
                # Took care of the one node passed so get out.
                break

        return True

# Deprecated TestAWSSMKill, we use TestSMKill for AWS nodes too TODO: pooja remove class
# This class contains the attributes and methods to test
# killing an (SM) service.
class TestAWSSMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_SMKill,
                                             "SM service kill  ")

        self.passedNode = node

    def test_AWS_SMKill(self):
        """
        Test Case:
        Attempt to kill the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("sm") == 0:
                self.log.warning("SM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Killing SM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('StorMgr', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            sm_obj = FDSServiceUtils.SMService(om_ip, node_ip)
            ret_status = sm_obj.kill(node_ip)

            if ret_status:
                status = 0
                self.log.info("SM (StorMgr) has been killed and restarted on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to kill SM (StorMgr) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# killing an (SM) service.
class TestAWSSMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_SMStop,
                                             "Stop SM service  ")

        self.passedNode = node

    def test_AWS_SMStop(self):
        """
        Test Case:
        Attempt to stop the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Stopping SM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('StorMgr', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            sm_obj = FDSServiceUtils.SMService(om_ip, node_ip)
            ret_status = sm_obj.stop(node_ip)

            if ret_status:
                status = 0
                self.log.info("SM (StorMgr) is no longer running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to stop SM (StorMgr) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

# This class contains the attributes and methods to test
# killing an (SM) service.
class TestAWSSMStart(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_SMStart,
                                             "Start SM service  ")

        self.passedNode = node

    def test_AWS_SMStart(self):
        """
        Test Case:
        Attempt to start the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Starting SM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('StorMgr', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            sm_obj = FDSServiceUtils.SMService(om_ip, node_ip)
            ret_status = sm_obj.start(node_ip)

            if ret_status:
                status = 0
                self.log.info("SM (StorMgr) is running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to start SM (StorMgr) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

# This class contains the attributes and methods to test
# adding Storage Manager (SM) service.
class TestAWSSMAdd(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_AddSMService,
                                             "Add SM service  ")

        self.passedNode = node

    def test_AWS_AddSMService(self):
        """
        Test Case:
        Attempt to add the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Adding SM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            sm_obj = FDSServiceUtils.SMService(om_ip, node_ip)
            ret_status = sm_obj.add(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# removing Storage Manager (SM) service.
class TestAWSSMRemove(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_RemoveSMService,
                                             "Remove SM service  ")

        self.passedNode = node

    def test_AWS_RemoveSMService(self):
        """
        Test Case:
        Attempt to remove the SM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Removing SM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            sm_obj = FDSServiceUtils.SMService(om_ip, node_ip)
            ret_status = sm_obj.remove(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
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
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Verify the SM on %s is down." %n.nd_conf_dict['node-name'])

            if not modWait("StorMgr", n, forShutdown=True):
                pid = getSvcPIDforNode('StorMgr', n)
                if pid != -1 and pid == sm_killed_pid[n.nd_conf_dict['node-name']] :
                    return False
                self.log.info("The SM on %s was down and restarted by platformd." %n.nd_conf_dict['node-name'])

            if self.passedNode is not None:
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

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we spin through in defined node setting it up.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)
            else:
                # Skip the PM for the OM's node. That one is handled by TestPMForOMBringUp()
                if n.nd_conf_dict['node-name'] == om_node.nd_conf_dict['node-name']:
                    self.log.info("Skipping OM's PM on %s." % n.nd_conf_dict['node-name'])
                    continue

            bin_dir = n.nd_agent.get_bin_dir(debug=False)
            log_dir = n.nd_agent.get_log_dir()

            self.log.info("Start PM on %s." % n.nd_conf_dict['node-name'])

            status = n.nd_start_platform(om_ip, test_harness=True, _bin_dir=bin_dir, _log_dir=log_dir)

            if status != 0:
                self.log.error("PM on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
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
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure it wasn't the target of a random kill
            if (n, "pm") in self.parameters.get('svc_killed', []):
                self.log.warning("PM service for node {} previously "
                                 "killed by random svc kill test.".format(n.nd_conf_dict['node-name']))
                if self.passedNode is not None:
                    break
                else:
                    continue

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
                n = findNodeFromInv(nodes, self.passedNode)
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

# Deprecated TestAWSPMKill, we use TestPMKill for AWS nodes too TODO: pooja remove class
# This class contains the attributes and methods to test
# killing an (SM) service.
class TestAWSPMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_PMKill,
                                             "PM service kill  ")

        self.passedNode = node

    def test_AWS_PMKill(self):
        """
        Test Case:
        Attempt to kill the PM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Killing PM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('platformd', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            pm_obj = FDSServiceUtils.PMService(om_ip, node_ip)
            ret_status = pm_obj.kill(node_ip)

            if ret_status:
                status = 0
                self.log.info("PM (platformd) has been killed and restarted on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to kill PM (platformd) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# killing an (PM) service.
class TestAWSPMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_PMStop,
                                             "Stop PM service  ")

        self.passedNode = node

    def test_AWS_PMStop(self):
        """
        Test Case:
        Attempt to stop the PM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Stopping PM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('platformd', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            pm_obj = FDSServiceUtils.PMService(om_ip, node_ip)
            ret_status = pm_obj.stop(node_ip)

            if ret_status:
                status = 0
                self.log.info("PM (platformd) is no longer running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to stop PM (platformd) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# killing an (PM) service.
class TestAWSPMStart(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_PMStart,
                                             "Stop PM service  ")

        self.passedNode = node

    def test_AWS_PMStart(self):
        """
        Test Case:
        Attempt to start the PM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Starting PM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('platformd', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            pm_obj = FDSServiceUtils.PMService(om_ip, node_ip)
            ret_status = pm_obj.start(node_ip)

            if ret_status:
                status = 0
                self.log.info("PM (platformd) is running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to start PM (platformd) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
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
                n = findNodeFromInv(nodes, self.passedNode)
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

        om_node = fdscfg.rt_om_node
        bin_dir = om_node.nd_agent.get_bin_dir(debug=False)
        log_dir = om_node.nd_agent.get_log_dir()

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

            # Make sure it wasn't the target of a random kill
            if (om_node, "pm") in self.parameters.get('svc_killed', []):
                self.log.warning("PM service for node {} previous "
                                 "killed by random svc kill test. PASSING test.".format(om_node.nd_conf_dict['node-name']))
                return True

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

        # If we were passed a node and it is not the OM node
        # captured during config parsing, just exit.
        if self.passedNode is not None:
            self.passedNode = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedNode)
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Start OM on %s." % om_node.nd_conf_dict['node-name'])
            bin_dir = om_node.nd_agent.get_bin_dir(debug=False)
            log_dir = om_node.nd_agent.get_log_dir()

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
            self.passedNode = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedNode)
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:

            # Make sure it wasn't the target of a random kill
            if (om_node, "om") in self.parameters.get('svc_killed', []):
                self.log.warning("OM service for node {} previous "
                                 "killed by random svc kill test. PASSING test!".format(om_node.nd_conf_dict['node-name']))
                return True

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
            self.passedNode = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedNode)
            om_node = None
            if self.passedNode.nd_conf_dict['node-name'] == fdscfg.rt_om_node.nd_conf_dict['node-name']:
                om_node = self.passedNode
            else:
                self.log.warn("Node %s not configured for an OM service." % self.passedNode.nd_conf_dict['node-name'])
        else:
            om_node = fdscfg.rt_om_node

        if om_node is not None:
            self.log.info("Kill OM on %s." % om_node.nd_conf_dict['node-name'])

            status = om_node.nd_agent.exec_wait('pkill -KILL -f com.formationds.om.Main')

            # Probably we get the -9 return because pkill for a java process will
            # not find it based on the class name alone. See getSvcPIDforNode().
            # For a remote install, a failed kill (because its already dead) returns 1.
            if (status != -9) and (status != 0) and (om_node.nd_agent.env_install and status != 1):
                self.log.error("OM (com.formationds.om.Main) kill on %s returned status %d." %
                               (om_node.nd_conf_dict['node-name'], status))
                return False

            status = om_node.nd_agent.exec_wait("pkill -KILL orchMgr")

            if (status != 1) and (status != 0):
                self.log.error("OM (orchMgr) kill on %s returned status %d." %
                               (om_node.nd_conf_dict['node-name'], status))
                return False

        return True

# Deprecated TestAWSOMKill, we use TestOMKill for AWS nodes too TODO: pooja remove class
# This class contains the attributes and methods to test
# killing an (OM) service.
class TestAWSOMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_OMKill,
                                             "OM service kill  ")

        self.passedNode = node

    def test_AWS_OMKill(self):
        """
        Test Case:
        Attempt to kill the OM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("om") == 0:
                self.log.warning("OM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Killing OM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            om_obj = FDSServiceUtils.OMService(om_ip, node_ip)
            ret_status = om_obj.kill(om_ip)

            if ret_status:
                status = 0
                self.log.info("OM (om.Main) has been killed and restarted on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to kill OM (om.Main) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

# This class contains the attributes and methods to test
# killing an (OM) service.
class TestAWSOMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_OMStop,
                                             "Stop OM service  ")

        self.passedNode = node

    def test_AWS_OMStop(self):
        """
        Test Case:
        Attempt to stop the OM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Stopping OM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            om_obj = FDSServiceUtils.OMService(om_ip, node_ip)
            ret_status = om_obj.stop(om_ip)

            if ret_status:
                status = 0
                self.log.info("OM (om.Main) is no longer running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to stop OM (om.Main) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

# This class contains the attributes and methods to test
# killing an (OM) service.
class TestAWSOMStart(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_OMStart,
                                             "Start OM service  ")

        self.passedNode = node

    def test_AWS_OMStart(self):
        """
        Test Case:
        Attempt to start the OM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("om") == 0:
                self.log.warning("OM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Starting OM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            om_obj = FDSServiceUtils.OMService(om_ip, node_ip)
            ret_status = Om_obj.start(om_ip)

            if ret_status:
                status = 0
                self.log.info("OM (om.Main) is running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to start OM (om.Main) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

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
class TestAMBringUp(TestCase.FDSTestCase):
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
        nodes = fdscfg.rt_obj.cfg_nodes

        for n in nodes:
            # If we were passed a node, use it and get out. Otherwise,
            # we boot all AMs we have.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

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
            bin_dir = n.nd_agent.get_bin_dir(debug=False)
            log_dir = n.nd_agent.get_log_dir()

            # The AMAgent script expected to be invoked from the bin directory in which it resides.
            status = n.nd_agent.exec_wait('bash -c \"(nohup ./AMAgent --fds-root=%s 0<&- &> %s/am.%s.out &) \"' %
                                          (fds_dir, log_dir, port),
                                          fds_bin=True)

            if status != 0:
                self.log.error("AM on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False
            elif self.passedNode is not None:
                # We were passed a specific AM so get out now.
                return True

        return True


# This class contains the attributes and methods to test
# activating Access Manager (AM) services on a domain.
class TestAMActivate(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMActivate,
                                             "AM service activation")

        self.passedNode = node

    def test_AMActivate(self):
        """
        Test Case:
        Attempt to activate the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']
        node_service = get_node_service(self,om_ip)
        node_list = node_service.list_nodes()

        for each_node in node_list:

            # Do all AMs in the domain or just the AM for the passed node?
            if self.passedNode is not None:
                self.log.info("Activate AM for node %s" % each_node.id)

                # Make sure this node is configured for an AM service.
                if self.passedNode.nd_services.count("am") == 0:
                    self.log.error("Node %s is not configured to host an AM service." % self.passedNode.nd_conf_dict['node-name'])
                    return False

                # Make sure we have the node's meta-data.
                status = self.passedNode.nd_populate_metadata(om_node=om_node)
                if status != 0:
                    self.log.error("Getting meta-data for node %s returned status %d." %
                               (each_node.nd_conf_dict['node-name'], status))
                    return False

                node_id = int(self.passedNode.nd_uuid, 16)
                each_node = node_service.get_node(node_id)

            service = Service()
            service.type = 'AM'
            add_service = node_service.add_service(each_node.id,service)
            if type(add_service).__name__ == 'FdsError':
                self.log.error(" Add AM service failed on node %s"%(each_node.id))
                return False

            self.log.info("Activate service AM for node %s." % (each_node.id))
            start_service = node_service.start_service(each_node.id,add_service.id)
            time.sleep(3)
            get_service = node_service.get_service(each_node.id,start_service.id)
            if type(get_service).__name__ == 'FdsError' or get_service.status.state == "NOT_RUNNING":
                self.log.error("AM Service activation of node %s returned status %s." %
                        (each_node.id, get_service))
                return False

            if self.passedNode is not None:
                # If we were passed a specific node, activated AM on that node so exit now.
                break

        return True


# This class contains the attributes and methods to test
# stopping Access Manager (AM) services on a domain.
# We don't currently have test cases that remove AM yet, as the need for it has not arisen.
class TestAMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AMStop,
                                             "AM service stopping")

        self.passedNode = node

    @TestCase.expectedFailure
    def test_AMStop(self):
        """
        Test Case:
        Attempt to stop the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        om_ip = om_node.nd_conf_dict['ip']
        nodes = fdscfg.rt_obj.cfg_nodes
        for node in nodes:
            # If we were passed a node, check that one and exit.
            if self.passedNode is not None:
                node = self.passedNode

            # Make sure this node is configured for an AM service.
            if (node.nd_services.count("am") == 0) and self.passedNode is not None:
                self.log.error("Node %s is not configured to host a AM service." % node.nd_conf_dict['node-name'])
                return False

            # Make sure we have the node's meta-data.
            status = node.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (node.nd_conf_dict['node-name'], status))
                return False

            self.log.info("Stopping AM for node %s using OM node %s." % (node.nd_conf_dict['node-name'],
                                                                        om_node.nd_conf_dict['node-name']))
            node_service = get_node_service(self, om_ip)
            node_cli = node_service.get_node(int(node.nd_uuid, 16))
            try:
                am_state = node_cli.services['AM'][0].status.state
                if am_state == 'RUNNING':
                    #if am is running then only stop it
                    am_service = node_service.stop_service(node_cli.id,node_cli.services['SM'][0].id)
                    am_state = am_service.status.state
                assert am_state == "NOT_RUNNING"
            except IndexError:
                self.log.error("Active AM service not found on %s ." % (node.nd_conf_dict['node-name']))
                return False

            if self.passedNode is not None:
                break

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
                self.passedNode = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedNode)
                # But only check it if it should have an AM.
                if self.passedNode.nd_conf_dict['node-name'] != n.nd_conf_dict['node-name']:
                    continue

            # Make sure it wasn't the target of a random kill
            if (n, "am") in self.parameters.get('svc_killed', []):
                self.log.warning("AM service for node {} previously "
                                 "killed by random svc kill test.".format(n.nd_conf_dict['node-name']))
                if self.passedNode is not None:
                    break
                else:
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
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("am") == 0:
                self.log.warning("AM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Shutdown AM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('bare_am', n)
            if pid != -1:
                cmd = "kill -KILL %s" % pid
                status = n.nd_agent.exec_wait(cmd)

                if status != 0:
                    self.log.error("AM (bare_am) shutdown on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                    return False
            else:
                status = 0
                self.log.warning("AM (bare_am) already shutdown on %s." % (n.nd_conf_dict['node-name']))
            bare_am_killed_pid[n.nd_conf_dict['node-name']]= pid

            # java AM (com.formationds.am.Main) is dependent on bare_am.
            # kill/restart of bare_am cause kill/restart of java_am hence sleep for couple of seconds
            time.sleep(2)
            pid = getSvcPIDforNode('java', n, javaClass='com.formationds.am.Main')
            if pid != -1:
               cmd = "kill -KILL %s" % pid
               status = n.nd_agent.exec_wait(cmd)
            if status != 0:
                self.log.error("AM (com.formationds.am.Main) shutdown on %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                return False
            else:
                status = 0
                self.log.warning("AM (com.formationds.am.Main) already shutdown on %s." % (n.nd_conf_dict['node-name']))

            if (status != 1) and (status != 0):
                self.log.error("AM shutdown on %s returned status %d." % (n.nd_conf_dict['node-name'], status))
                return False

            if self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

# Deprecated TestAWSAMKill, we use TestAMKill for AWS nodes too TODO: pooja remove class
# This class contains the attributes and methods to test
# killing an Access Manager (AM) service.
class TestAWSAMKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_AMKill,
                                             "AM service kill  ")

        self.passedNode = node

    def test_AWS_AMKill(self):
        """
        Test Case:
        Attempt to kill the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            # Make sure there is supposed to be an AM service on this node.
            if n.nd_services.count("am") == 0:
                self.log.warning("AM service not configured for node %s." % n.nd_conf_dict["node-name"])
                if self.passedNode is None:
                    continue
                else:
                    break

            self.log.info("Killing AM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('bare_am', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            am_obj = FDSServiceUtils.AMService(om_ip, node_ip)
            ret_status = am_obj.kill(node_ip)

            if ret_status:
                status = 0
                self.log.info("AM (bare_am) has been killed and restarted on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to kill AM (bare_am) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# killing an Access Manager (AM) service.
class TestAWSAMStop(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_AMStop,
                                             "Stop AM service  ")

        self.passedNode = node

    def test_AWS_AMStop(self):
        """
        Test Case:
        Attempt to stop the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Stopping AM on %s." % n.nd_conf_dict['node-name'])

            # Get the PID of the processes in question and ... kill them!
            pid = getSvcPIDforNode('bare_am', n)

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            am_obj = FDSServiceUtils.AMService(om_ip, node_ip)
            ret_status = am_obj.stop(node_ip)

            if ret_status:
                status = 0
                self.log.info("AM (bare_am) is no longer running on %s." % (n.nd_conf_dict['node-name']))
            else:
                self.log.error("Failing to stop AM (bare_am) service on %s" %
                                (n.nd_conf_dict['node-name']))

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# starting an Access Manager (AM) service.
class TestAWSAMStart(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_AMStart,
                                             "Start AM service  ")

        self.passedNode = node

    def test_AWS_AMStart(self):
        """
        Test Case:
        Attempt to start the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Startinng AM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            am_obj = FDSServiceUtils.AMService(om_ip, node_ip)
            ret_status = am_obj.start(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# adding an Access Manager (AM) service.
class TestAWSAMAdd(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_AddAMService,
                                             "Add AM service  ")

        self.passedNode = node

    def test_AWS_AddAMService(self):
        """
        Test Case:
        Attempt to add the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Adding AM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            am_obj = FDSServiceUtils.AMService(om_ip, node_ip)
            ret_status = am_obj.add(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
                return False
            elif self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True


# This class contains the attributes and methods to test
# remove an Access Manager (AM) service.
class TestAWSAMRemove(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_AWS_RemoveAMService,
                                             "Remove AM service  ")

        self.passedNode = node

    def test_AWS_RemoveAMService(self):
        """
        Test Case:
        Attempt to remove the AM service(s)
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            # If a specific node was passed in, use that one and get out.
            if self.passedNode is not None:
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Removing AM on %s." % n.nd_conf_dict['node-name'])

            om_node = fdscfg.rt_om_node
            om_ip = om_node.nd_conf_dict['ip']
            node_ip = n.nd_conf_dict['ip']
            am_obj = FDSServiceUtils.AMService(om_ip, node_ip)
            ret_status = am_obj.remove(node_ip)

            if ret_status:
                status = 0

            if (status != 0):
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
                n = findNodeFromInv(nodes, self.passedNode)

            self.log.info("Verify AM on %s is down" % n.nd_conf_dict['node-name'])
            services = n.nd_services.split(",")
            if 'am' not in services:
                self.log.info("AM is not available for %s" %n.nd_conf_dict['node-name'])
                continue

            if not modWait("bare_am", n, forShutdown=True):
                pid = getSvcPIDforNode('bare_am', n)
                if pid != -1 and pid == bare_am_killed_pid[n.nd_conf_dict['node-name']]:
                    return False
                self.log.info("The AM on %s was down and was probably restarted by PM with new pid %s" %(n.nd_conf_dict['node-name'],pid))

            if self.passedNode is not None:
                # We took care of the one node. Get out.
                break

        return True

#This class injects fault in Storage Manager running on a given node.
#It randomly chooses a fault to inject from a given set of faults passed
#as parameter to the testcase in the system test.
class TestServiceInjectFault(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node='random_node', service=None, faultName=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ServiceFaultInjection,
                                             "Test setting fault injection")
        self.passedNode = node
        self.passedService = service
        self.passedFaultName = faultName.split(' ')

    def test_ServiceFaultInjection(self):
        """
        Test Case:
        This testcase sets the fault injection parameter on specific service in a node
        At the moment only random_node is supported
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        svc_map = plat_svc.SvcMap(fdscfg.rt_om_node.nd_conf_dict['ip'],
                                  fdscfg.rt_om_node.nd_conf_dict['fds_port'])
        svcs = svc_map.list()

        if self.passedNode is not None and self.passedNode != "random_node":
            self.passedNode = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.passedNode)
            if self.passedNode is None:
                # throw error
                self.log.error("Unable to find node from inventory")
            # Sometimes the node UUID doesn't get populated. This call makes sure that it does.
            status = self.passedNode.nd_populate_metadata(om_node=om_node)
            passed_node_uuid = self.passedNode.nd_uuid

            # First filter out only services belonging to the specified node
            # Use a little voodoo to match service UUIDs to the node UUID
            svcs = filter(lambda x: str(x[0])[:-1] == str(long(passed_node_uuid, 16))[:-1], svcs)
            if not svcs:
                self.log.error("Unable to find the service belonging to such node")
                return False;

        '''
        Randomly choose a fault to inject
        '''
        chosenFault = random.choice(self.passedFaultName)

        if self.passedService is None:
            # Pick out the am/sm/dm/om/pm from the service injection name
            m = re.search('(a|s|d|o|p)m', chosenFault)
            if m is not None:
                self.passedService = m.group(0)
            else:
                self.log.error("Unable to automatically parse module name from fault injection name")
                return False

        if self.passedNode is not None and self.passedNode != "random_node":
            self.log.info("Selected {} as random fault to inject for {} on node {} ".format(
                chosenFault, self.passedService, self.passedNode.nd_conf_dict['node-name']))
        else:
            self.log.info("Selected {} as random fault to inject for {} on node {} ".format(
                chosenFault, self.passedService, self.passedNode))

        # Svc map will be a list of lists in the form:
        # [ [uuid, svc_name, ???, ip, port, is_active?] ]
        if self.passedService not in svcs:
            self.log.info("Not in svcs... retrying")
            status = self.passedNode.nd_populate_metadata(om_node=om_node)
            passed_node_uuid = self.passedNode.nd_uuid
            svcs = filter(lambda x: str(x[0])[:-1] == str(long(passed_node_uuid, 16))[:-1], svcs)

        svc_uuid = filter(lambda x: self.passedService in x, svcs)[0][0]
        if not svc_uuid:
            self.log.error("Unable to find the servcie UUID")
            return False

        # set the actual injection
        res = svc_map.client(svc_uuid).setFault('enable name=' + chosenFault)
        if res:
            return True

        return False

class TestStorMgrDeleteDisk(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None, disk=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DeleteDisk,
                                             "Test to remove disk from fds mounted directory")
        self.node = node
        self.disk = disk
    def test_DeleteDisk(self):
        fdscfg = self.parameters["fdscfg"]
        self.node = findNodeFromInv(fdscfg.rt_obj.cfg_nodes, self.node)
        fds_dir = self.node.nd_conf_dict['fds_root']
        diskPath = os.path.join(fds_dir, 'dev', self.disk)
        self.log.info("Going to delete mount point {} for disk {}".format(diskPath, self.disk))
        shutil.rmtree(diskPath)
        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output='test-reports'))
