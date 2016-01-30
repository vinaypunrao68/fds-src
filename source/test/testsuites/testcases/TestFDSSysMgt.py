#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import TestCase
import re

# Module-specific requirements
import sys
from TestFDSServiceMgt import TestAMKill, TestSMKill, TestDMKill, TestOMKill, TestPMKill
from fdslib.TestUtils import get_node_service
from fdscli.model.platform.service import Service
from fdslib.TestUtils import get_localDomain_service
import time
from fdscli.model.fds_error import FdsError
from fdslib.TestUtils import node_is_up
from fabric.contrib.files import *
from fdslib.TestUtils import disconnect_fabric
from fdslib.TestUtils import connect_fabric
from fdslib.TestUtils import get_log_count_dict

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

                if not self.expect_to_fail:
                    self.log.info("Activate service %s for node %s." % (service_name, n.nd_conf_dict['node-name']))
                    start_service = node_service.start_service(node_id,add_service.id)
                    time.sleep(3)
                    get_service = node_service.get_service(node_id,start_service.id)
                    if isinstance(get_service, FdsError) or get_service.status.state == "NOT_RUNNING":
                        self.log.error("Service activation of node %s returned status state %s." %
                                   (n.nd_conf_dict['node-name'], get_service.status.state))
                        return False

            if self.passedNode is not None:
                # If we were passed a specific node, exit now.
                return True

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

            node_state = node_service.get_node(node_id)
            service_list = ['AM', 'DM', 'SM', 'PM']
            for service_name in service_list:
                if (node_state.services['{}'.format(service_name)][0].status.state != "RUNNING") and node_state.state != 'UP':
                    self.log.warn("FAILED:  Expected Node service=RUNNING, Returned node service={}".format(node_state.services['{}'.format(service_name)][0].status.state))
                    return False

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
        local_domain_service = get_localDomain_service(self,om_node.nd_conf_dict['ip'])
        domains = local_domain_service.get_local_domains()
        domain = domains[0]

        status = local_domain_service.start(domain)

        if isinstance(status, FdsError):
            self.log.error("Domain startup returned status %d." % (status))
            return False

        # Delay for domain restart
        time.sleep(3)
        return True

# Create domain isn't a thing that exists yet 10/30/15: Deprecated class for now
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


# This class contains the attributes and methods to test
# node removal from domain and verifying node is in good state after removal.
class TestNodeRemove(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None, service_list=None, logentry_list=None, maxwait=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeRemove,
                                             "Remove a node from domain")

        self.passedNode = node
        self.passedService_list = service_list
        self.passedLogentry_list = logentry_list
        self.maxwait = maxwait

    def test_NodeRemove(self):
        service_list = self.passedService_list
        log_entry_list = self.passedLogentry_list
        om_node = self.parameters["fdscfg"].rt_om_node
        node_ip = self.passedNode.nd_conf_dict['ip']

        # store passed log entry count before removing a node
        pre_remove_count = get_log_count_dict(self,om_node.nd_conf_dict['ip'], node_ip, service_list, log_entry_list)

        self.log.info("Removing %s from domain." % self.passedNode.nd_conf_dict['ip'])
        node_service = get_node_service(self,om_node.nd_conf_dict['ip'])

        assert self.passedNode.nd_populate_metadata(om_node=om_node) is 0
        node_id = int(self.passedNode.nd_uuid, 16)
        remove_node_status = node_service.remove_node(node_id)
        time.sleep(3) # Give some time to cli to update node state
        if isinstance(remove_node_status, FdsError) or type(remove_node_status).__name__ == 'FdsError':
            return False
        else:
            node = node_service.get_node(node_id)
            assert str(node.state) == 'DISCOVERED'
            verify_remove = verify_with_logs(self, node_ip, service_list, log_entry_list, self.maxwait, pre_remove_count)
            return True if verify_remove else False


# This class contains the attributes and methods to test
# rebooting node (I.e. node shuts down and comes back up again)
class TestNodeReboot(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node_ip=None, service_list=None, logentry_list=None, maxwait =None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeReboot,
                                             "Reboot a node")

        self.passedNode_ip = node_ip
        self.passedService_list = service_list
        self.passedLogentry_list = logentry_list
        self.maxwait = maxwait

    def test_NodeReboot(self):
        """
        Test Case:
        Attempt to reboot a given node.
        """
        service_list = self.passedService_list
        log_entry_list = self.passedLogentry_list
        node_ip = self.passedNode_ip
        om_node = self.parameters["fdscfg"].rt_om_node

        pre_reboot_count = get_log_count_dict(self,om_node.nd_conf_dict['ip'],node_ip,service_list,log_entry_list)
        self.log.info("%s is going down for reboot NOW!" % self.passedNode_ip)
        assert connect_fabric(self, node_ip) is True
        run('reboot')
        disconnect_fabric()

        # This if will confirm that node is back up again after reboot
        if not connect_fabric(self, node_ip):
            return False
        else:
            verify_reboot = verify_with_logs(self, node_ip, service_list, log_entry_list, self.maxwait, pre_reboot_count)
            return True if verify_reboot else False


# This method to verifies if node is in good state for IO after reboot
# by confirming passed log entry count has increased after reboot.
def verify_with_logs(self, node_ip, service_list, log_entry_list, maxwait, pre_count_dict):
    assert connect_fabric(self, node_ip) is True
    disconnect_fabric()
    wait_for_log_time = maxwait  # It's in minutes
    om_node = self.parameters["fdscfg"].rt_om_node

    # Given wait time is in minutes, if logs after reboot are not greater than logs before reboot then
    # we sleep for 30 secs and check again, hence wait_for_log_time is multiplied by 2
    for i in range(0, wait_for_log_time * 2):
        post_count_dict = get_log_count_dict(self,om_node.nd_conf_dict['ip'],node_ip, service_list, log_entry_list)
        post_list = list(post_count_dict.values())
        pre_list = list(pre_count_dict.values())
        status = False
        for i in range(len(pre_list)):
            if not post_list[i] > pre_list[i]:
                status = False
                break
            else:
                status = True
                continue

        if status:
            self.log.info("SUCCESS. For log_entry_list %s -  post_reboot_count: %s , pre_reboot_count: %s "
                          % (log_entry_list,post_list, pre_list))
            return True
        else:
            # before rechecking logs wait for 30 sec
            time.sleep(30)
            print 'Rechecking logs .. '
            continue
    self.log.error('FAILED. Log verification failed after %s min retires too.' %maxwait)
    return False


class TestNodeAdd(TestCase.FDSTestCase):
    def __init__(self, parameters=None, node=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_NodeAdd,
                                             "Add node in domain")

        self.passedNode = node

    def test_NodeAdd(self):
        """
        Test Case:
        Attempt to activate the services of the specified node(s).
        """

        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node
        nodes = fdscfg.rt_obj.cfg_nodes
        om_ip = om_node.nd_conf_dict['ip']

        for n in nodes:
            # If we were provided a node, activate that one and exit.
            if self.passedNode is not None:
                n = self.passedNode

            status = n.nd_populate_metadata(om_node=om_node)
            if status != 0:
                self.log.error("Getting meta-data for node %s returned status %d." %
                               (n.nd_conf_dict['node-name'], status))
                return False

            # As of 01/19/2015, after removing a node from domain it goes in DISCOVERED state and
            # none of the services are running on removed node, However via cli we can see removed
            # node's PM in DISCOVERD state, we add back node using cli- POOJA
            node_id = int(n.nd_uuid, 16)
            node_service = get_node_service(self,om_ip)
            node = node_service.get_node(node_id)
            # Verify we are adding node which is in discovered state
            assert str(node.state) == 'DISCOVERED'
            status = node_service.add_node(node_id, node)
            time.sleep(3)
            assert node_is_up(self,om_ip, node_id) is True

            if type(status).__name__ == 'FdsError':
                self.log.error("Adding node %s returned status %s." %(node_id,status))
                return False
            if self.passedNode is not None:
                # If we were passed a specific node, exit now.
                return True

        return True


if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
