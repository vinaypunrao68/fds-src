#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback
import TestCase
import pdb
import re
import time

# Module-specific requirements
import sys
import os
import random
import time
from TestFDSServiceMgt import TestAMKill, TestSMKill, TestDMKill, TestOMKill, TestPMKill
from TestFDSServiceMgt import TestAMBringUp
from fdslib.TestUtils import get_node_service
from fdslib.TestUtils import get_node_state
from fdscli.model.platform.service import Service
from fdscli.model.platform.node import Node
from fdslib.TestUtils import get_localDomain_service
import time
from fdscli.model.fds_error import FdsError
from fdslib.TestUtils import node_is_up

# This class contains the attributes and methods to test
# activation of an FDS domain starting the same, specified
# services on each node.
class TestDomainActivateServices(TestCase.FDSTestCase):
    def __init__(self, parameters=None, services="dm,sm,am"):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DomainActivateServices,
                                             "Domain activation")

        self.passedServices = services

    def test_DomainActivateServices(self):
        """
        Test Case:
        Attempt to activate services in a domain.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]

        om_node = fdscfg.rt_om_node
        #TODO:POOJA once we re done with fs-3007 , fdscli can be used ot activate the domain, this is temp work around
        nodes = fdscfg.rt_obj.cfg_nodes
        om_ip = om_node.nd_conf_dict['ip']
        for n in nodes:
            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                    (n.nd_conf_dict['node-name'], status))
                return False
            node_id = int(n.nd_uuid, 16)
            services = self.passedServices.split(",")
            for service_name in services:
                service_name = service_name.upper()
                node_service = get_node_service(self,om_ip)
                service = Service()
                service.type = service_name
                add_service = node_service.add_service(node_id,service)
                if type(add_service).__name__ == 'FdsError':
                    self.log.error(" Add service %s failed on node %s with"%(service_name, n.nd_conf_dict['node-name']))
                    return False
                self.log.info("Activate service %s for node %s." % (service_name, n.nd_conf_dict['node-name']))
                start_service = node_service.start_service(node_id,add_service.id)
                time.sleep(3)
                get_service = node_service.get_service(node_id,start_service.id)
                if isinstance(get_service, FdsError) or get_service.status.state == "NOT_RUNNING":
                    self.log.error("Service activation of node %s returned status %d." %
                        (n.nd_conf_dict['node-name'], status))
                    return False
        return True


# This class contains the attributes and methods to test
# the kill the services of an FDS node.
class TestNodeKill(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeKill,
                                             "Node kill",
                                             True)  # Always run.

        self.passedNode = node

    def test_NodeKill(self, ansibleBoot=False):
        """
        Test Case:
        Attempt to kill all services on a node.

        We try to kill just the services defined for a node unless we're killing
        an Ansible boot in which case we go after all services on all nodes.
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

            # First kill PM to prevent respawn of other services
            killPM = TestPMKill(node=n)
            killSuccess = killPM.test_PMKill()

            if not killSuccess:
                self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                return False


            # Then kill AM if on this node.
            if (n.nd_services.count("am") > 0) or ansibleBoot:
                killAM = TestAMKill(node=n)
                killSuccess = killAM.test_AMKill()

                if not killSuccess and not ansibleBoot:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            # SM and DM next.
            if (n.nd_services.count("sm") > 0) or ansibleBoot:
                killSM = TestSMKill(node=n)
                killSuccess = killSM.test_SMKill()

                if not killSuccess and not ansibleBoot:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            if (n.nd_services.count("dm") > 0) or ansibleBoot:
                killDM = TestDMKill(node=n)
                killSuccess = killDM.test_DMKill()

                if not killSuccess and not ansibleBoot:
                    self.log.error("Node kill on %s failed." % (n.nd_conf_dict['node-name']))
                    return False

            # Lastly, kill OM if on this node.
            if (fdscfg.rt_om_node.nd_conf_dict['node-name'] == n.nd_conf_dict['node-name']) or ansibleBoot:
                killOM = TestOMKill(node=n)
                killSuccess = killOM.test_OMKill()

                if not killSuccess and not ansibleBoot:
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
    def __init__(self, parameters=None, node=None, expect_to_fail=False, expect_failed_msg=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeActivateService,
                                             "Node services activation")

        self.passedNode = node
        self.expect_to_fail = expect_to_fail
        self.expect_failed_msg = expect_failed_msg

    def test_NodeActivateService(self):
        """
        Test Case:
        Attempt to activate the services of the specified node(s).
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        fds_dir = om_node.nd_conf_dict['fds_root']
        log_dir = om_node.nd_agent.get_log_dir()
        nodes = fdscfg.rt_obj.cfg_nodes
        om_ip = om_node.nd_conf_dict['ip']

        for n in nodes:
            # If we were provided a node, activate that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            # If we're asked to activate nodes, we should be able to spin through our nodes to obtain
            # some useful information about them.
            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False
            #To activate node we need to add all the services listed and then manually start each service
            node_id = int(n.nd_uuid, 16)
            services = n.nd_services.split(",")
            for service_name in services:
                service_name = service_name.upper()
                node_service = get_node_service(self,om_ip)
                service = Service()
                service.type = service_name
                add_service = node_service.add_service(node_id,service)
                if (type(add_service).__name__ == 'FdsError'):
                    if self.expect_to_fail:
                        if isinstance(add_service, FdsError):
                            self.log.error("FAILED:  Service activation on node %s returned status %d." %
                                (n.nd_conf_dict['node-name'], add_service.message))
                            return False

                        elif re.search(self.expect_failed_msg, add_service.message):
                            node_service = get_node_service(self,om_ip)
                            get_node = node_service.get_node(node_id)
                            self.log.info("PASSED:  Service activation on node={} is not allowed for service.id={}.".format(n.nd_conf_dict['node-name'], service.id))
                            self.log.info("PASSED:  Expect failed message=> {}, Found=> {}".format(self.expect_failed_msg, add_service.message))
                            return True

                    else:
                        self.log.error("Failing to add service {} to node {} returned status {}".format(service_name, n.nd_conf_dict['node-name'], add_service.message))
                        return False

                else:
                    self.log.error(" Add service %s failed on node %s with"%(service_name, n.nd_conf_dict['node-name']))
                    #return False

                if not self.expect_to_fail:
                    self.log.info("Activate service %s for node %s." % (service_name, n.nd_conf_dict['node-name']))
                    start_service = node_service.start_service(node_id,add_service.id)
                    time.sleep(3)
                    get_service = node_service.get_service(node_id,start_service.id)
                    if isinstance(get_service, FdsError) or get_service.status.state == "NOT_RUNNING":
                        self.log.error("Service activation of node %s returned status %d." %
                                   (n.nd_conf_dict['node-name'], status))
                        return False

            if self.passedNode is not None:
                # If we were passed a specific node, exit now.
                return True

        return True


# This class contains the attributes and methods to test
# services removal of nodes. (I.e. remove the node from
# the domain.)
class TestNodeRemoveServices(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeRemoveService,
                                             "Node services removal")

        self.passedNode = node

    def test_NodeRemoveService(self):
        """
        Test Case:
        Attempt to remove the services of nodes from a domain.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        nodes = fdscfg.rt_obj.cfg_nodes
        om_ip = om_node.nd_conf_dict['ip']
        for n in nodes:
        # If we were provided a node, deactivate that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            self.log.info("Removing node %s. " % n.nd_conf_dict['node-name'])

            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                    (n.nd_conf_dict['node-name'], status))
                return False

            node_id = int(n.nd_uuid, 16)
            # Prevent scenario where we try to remove a node that was never online
            if not node_is_up(self,om_ip,node_id):
                self.log.info("Selected node {} is not UP. Ignoring "
                                     "command to remove node".format(n.nd_conf_dict['node-name']))
                continue

            node_service = get_node_service(self,om_ip)
            node_remove = node_service.remove_node(node_id)

            #check node state after remove_node
            node_state = node_service.get_node(node_id)
            service_list = ['AM', 'DM', 'SM', 'PM']
            for service_name in service_list:
                if (node_state.services['{}'.format(service_name)][0].status.state != "RUNNING") and node_state.state != 'UP':
                    self.log.warn("FAILED:  Expected Node service=RUNNING, Returned node service={}".format(node_state.services['{}'.format(service_name)][0].status.state))
                    return False

            if isinstance(node_remove, FdsError) :
                self.log.error("Removal of node %s returned status %s." %
                               (n.nd_conf_dict['node-name'], node_remove))
                return False
            elif self.passedNode is not None:
                # If we were passed a specific node, exit now.
                break

        return True


# This class contains the attributes and methods to test
# node shutdown. (I.e. stop all services on the node)
class TestNodeShutdown(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeShutdown,
                                             "Stop all services on a node")

        self.passedNode = node

    def test_NodeShutdown(self):
        """
        Test Case:
        Attempt to shutdown the node.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        nodes = fdscfg.rt_obj.cfg_nodes
        om_ip = om_node.nd_conf_dict['ip']
        for n in nodes:
        # If we were provided a node, deactivate that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            self.log.info("Shutdown node %s. " % n.nd_conf_dict['node-name'])
            node_id = int(n.nd_uuid, 16)
            # Prevent scenario where we try to remove a node that was never online
            if not node_is_up(self,om_ip,node_id):
                self.log.info("Selected node {} is down already. Ignoring "
                                     "command to stop node".format(n.nd_conf_dict['node-name']))
                continue

            node_service = get_node_service(self,om_ip)
            node_shutdown= node_service.stop_node(node_id)
            time.sleep(5)
            #check node state after stop_node
            node_state = node_service.get_node(node_id)
            service_list = ['AM', 'DM', 'SM', 'PM']
            for service_name in service_list:
                if (node_state.services['{}'.format(service_name)][0].status.state != "NOT_RUNNING") and node_state.state != 'DOWN':
                    self.log.warn("FAILED:  Expected Node service=NOT_RUNNING, Returned node service={}".format(node_state.services['{}'.format(service_name)][0].status.state))
                    return False


            if isinstance(node_shutdown, FdsError) :
                self.log.error("Shutdown of node %s returned status %s." %
                               (n.nd_conf_dict['node-name'], node_shutdown))
                return False
            elif self.passedNode is not None:
                # If we were passed a specific node, exit now.
                break

        return True

# This class contains the attributes and methods to test
# start node (I.e. node start services on the node)
class TestNodeStart(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeStart,
                                             "Start all services on a node")

        self.passedNode = node

    def test_NodeStart(self):
        """
        Test Case:
        Attempt to start all services on a node.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        nodes = fdscfg.rt_obj.cfg_nodes
        om_ip = om_node.nd_conf_dict['ip']
        for n in nodes:
        # If we were provided a node, deactivate that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            self.log.info("Shutdown node %s. " % n.nd_conf_dict['node-name'])
            node_id = int(n.nd_uuid, 16)
            # Prevent scenario where we try to remove a node that was never online
            if node_is_up(self,om_ip,node_id):
                self.log.info("Selected node {} is UP already. Ignoring "
                                     "command to start node".format(n.nd_conf_dict['node-name']))
                continue

            node_service = get_node_service(self,om_ip)
            node_shutdown= node_service.start_node(node_id)

            node_state = get_node_state(self,om_ip, node_id)
            if node_state == 'UP':
                self.log.info("PASSED:  Node start on node {} was successful.  Expected Node state = UP, Returned Node state={}".format(n.nd_conf_dict['node-name'], node_state))

            else:
                self.log.error("FAILED:  Failing to start node {}.  Expected Node state = UP, Returned Node state={}".format(n.nd_conf_dict['node-name'], node_state))

            if isinstance(node_shutdown, FdsError) :
                self.log.error("Node start of node %s returned status %s." %
                               (n.nd_conf_dict['node-name'], node_shutdown))
                return False
            elif self.passedNode is not None:
                # If we were passed a specific node, exit now.
                break

        return True


# This class contains the attributes and methods to test
# domain shutdown
class TestDomainShutdown(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DomainShutdown,
                                             "Domain shutdown")

    def test_DomainShutdown(self):
        """
        Test Case:
        Attempt to execute domain-shutdown.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        local_domain_service = get_localDomain_service(self,om_node.nd_conf_dict['ip'])
        local_domains =local_domain_service.get_local_domains()
        for domain in local_domains:
            self.log.info("Shutdown domain.")
            status = local_domain_service.shutdown(domain)

        if isinstance(status, FdsError) or type(status).__name__ == 'FdsError':
            self.log.error("Domain shutdown returned status %s." % (status))
            return False
        else:
            return True

# This class contains the attributes and methods to test
# domain restart
class TestDomainStartup(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DomainStartup,
                                             "Domain startup")

    def test_DomainStartup(self):
        """
        Test Case:
        Attempt to execute domain-startup
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        self.log.info("Startup domain after shutdown.")

        status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py domain startup local) \"',
                                            fds_tools=True)

        if status != 0:
            self.log.error("Domain startup returned status %d." % (status))
            return False
        else:
            return True


# This class contains the attributes and methods to test
# domain create
class TestDomainCreate(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_DomainCreate,
                                             "Domain create")

    def test_DomainCreate(self):
        """
        Test Case:
        Attempt to execute domain-create.
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        self.log.info("Create domain.")

        status = om_node.nd_agent.exec_wait('bash -c \"(./fdsconsole.py domain create my_domain Woodville,MS) \"',
                                            fds_tools=True)

        if status != 0:
            self.log.error("Domain create returned status %d." % (status))
            return False
        else:
            return True

#This class removes a disk-map entry randomly
class TestRemoveDisk(TestCase.FDSTestCase):
    def __init__(self, parameters=None, diskMapPath=None, diskType=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_diskRemoval,
                                             "Testing disk removal")

        self.diskMapPath = diskMapPath
        self.diskType = diskType

    def test_diskRemoval(self):
        """
        Test Case:
        Remove a hdd/ssd chosen randomly from a given disk-map.
        """
        if (self.diskMapPath is None or self.diskType is None):
            self.log.error("Some parameters missing values {} {}".format(self.diskMapPath, self.diskType))
        self.log.info(" {} {} ".format(self.diskMapPath, self.diskType))
        if not os.path.isfile(self.diskMapPath):
            self.log.error("File {} not found".format(self.diskMapPath))
            return False
        diskMapFile = open(self.diskMapPath, "r+")
        diskEntries = diskMapFile.readlines()
        chosenDisks = filter(lambda x: self.diskType in x, diskEntries)
        chosenDisk = random.choice(chosenDisks)
        self.log.info("Disk to be removed {}".format(chosenDisk))
        diskMapFile.seek(0)
        for disk in diskEntries:
            if disk != chosenDisk:
                self.log.info(disk)
                diskMapFile.write(disk)
        diskMapFile.truncate()
        diskMapFile.seek(0)
        reRead = diskMapFile.readlines()
        self.log.info("Updated disk-map file {} \n".format(reRead))
        diskMapFile.close()
        return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
