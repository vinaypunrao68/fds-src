from __future__ import with_statement
from fabric import tasks
from fabric.api import *
from fabric.contrib.console import confirm
from fabric.contrib.files import *
from fabric.network import disconnect_all
from fabric.tasks import execute
import pdb
import re
import logging
import random
import time
import string
import sys
import TestUtils
from testcases import TestCase

from fdscli.services.node_service import NodeService
from fdscli.services.fds_auth import FdsAuth
from fdscli.services.users_service import UsersService
from fdscli.model.platform.node import Node
from fdscli.model.platform.service import Service 
from fdscli.model.platform.domain import Domain



random.seed(time.time())
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

class AMService(object):
    def __init__(self, om_ip, node_ip):
        #self.log_dir = fdscfg.rt_env.get_log_dir()
        #self.om_node =  fdscfg.rt_om_node
        #self.om_ip = self.om_node.nd_conf_dict['ip']
        self.om_ip =  om_ip
        self.node_ip =  node_ip
        env.user='root'
        env.password='passwd'
        env.host_string = self.om_ip
        TestUtils.create_fdsConf_file(self.om_ip)
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()

    def start(self, node_ip):
        '''
        Start AM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start AM service.

	    Returns:
	    -----------
	    oolean
        '''
        log.info(AMService.start.__name__)
        nodeNewState = Node()
        nodeNewState.am=True

        #start AM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.start_service(eachnode.id, eachnode.services['AM'][0].id)
                time.sleep(7)
                env.host_string = node_ip
                am_pid = sudo('pgrep bare_am')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    #if (eachnode.services['AM'][0].status.state ==  'ACTIVE' or eachnode.services['AM'][0].status.state ==  'RUNNING') and int(am_pid) > 0:
                    if (eachnode.services['AM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - AM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - AM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = AM service has not started  - unable to locate node.')
            return False
                

    def stop(self, node_ip):
        '''
        Stop AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop AM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(AMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.am=False
        NodeFound = False

        #stop AM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['AM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['AM'][0].status.state ==  'INACTIVE' or eachnode.services['AM'][0].status.state ==  'DOWN' or eachnode.services['AM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - AM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - AM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                             log.warn('WARNING - AM service is not available on node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('Fail - AM service has not started - unable to locate node')
            return False

    def kill(self, node_ip):
        '''
        Kill AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill AM service.

    	Returns:
    	-----------
    	Boolean
        '''
        env.host_string = node_ip
        am_pid_before = sudo('pgrep bare_am')

        log.info(AMService.kill.__name__)
        log.info('Killing bare_am service')
        sudo('pkill -9 bare_am')
        time.sleep(7)
        am_pid_after = sudo('pgrep bare_am')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['AM'][0].status ==  'ACTIVE':
                if am_pid_before ==  am_pid_after:
                    log.warn('FAIL - Failed to kill bare_am service on node {}'.format(eachnode.address.ipv4address))
                    return False 

                else:
                    log.info('PASS - bare_am service has been killed on node {}'.format(eachnode.address.ipv4address))
                    return True

    def add(self, node_ip):
        '''
        Add AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add AM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(AMService.add.__name__)
        log.info('Adding AM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="AM"
    	#newNodeService.type="FDSP_ACCESS_MGR"
        newNodeService.type="AM"
        #newNodeService.status="ACTIVE"
        #newNodeService.state="UP"
        newNodeService.status.state="NOT_RUNNING"

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.add_service(eachnode.id, newNodeService)

		time.sleep(7)
        env.host_string = node_ip

	    #check updated node state 
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    if eachnode.services['AM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added AM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add AM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add AM service to node {}'.format(eachnode.address.ipv4address))
                    return False

    def remove(self, node_ip):
        '''
        Remove AM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove AM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(AMService.remove.__name__)
        log.info('Removing AM service')
        nodeNewState = Node()
        nodeNewState.am=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['AM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                         log.warn('WARNING - AM service is not available on node {}'.format(eachnode.address.ipv4address))
                         return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['AM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['AM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['AM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove AM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed AM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - AM service has not been removed - unable to locate node')
            return False

class DMService(object):
    def __init__(self, om_ip, node_ip):
        #self.log_dir = fdscfg.rt_env.get_log_dir()
        #self.om_node =  fdscfg.rt_om_node
        #self.om_ip = self.om_node.nd_conf_dict['ip']
        self.om_ip =  om_ip
        self.node_ip =  node_ip
        env.user='root'
        env.password='passwd'
        env.host_string = self.om_ip
        TestUtils.create_fdsConf_file(self.om_ip)
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()

    def start(self, node_ip):
        '''
        Start DM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start DM service.

	    Returns:
	    -----------
	    oolean
        '''
        log.info(DMService.start.__name__)
        nodeNewState = Node()
        nodeNewState.dm=True

        #start DM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.start_service(eachnode.id, eachnode.services['DM'][0].id)
                time.sleep(7)
                env.host_string = node_ip
                dm_pid = sudo('pgrep DataMgr')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    if (eachnode.services['DM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - DM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - DM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = DM service has not started  - unable to locate node.')
            return False
                

    def stop(self, node_ip):
        '''
        Stop DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop DM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(DMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.dm=False
        NodeFound = False

        #stop DM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['DM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['DM'][0].status.state ==  'INACTIVE' or eachnode.services['DM'][0].status.state ==  'DOWN' or eachnode.services['DM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - DM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - DM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                             log.warn('WARNING - DM service is not available on node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('Fail - DM service has not started - unable to locate node')
            return False

    def kill(self, node_ip):
        '''
        Kill DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill DM service.

    	Returns:
    	-----------
    	Boolean
        '''
        env.host_string = node_ip
        dm_pid_before = sudo('pgrep DataMgr')

        log.info(DMService.kill.__name__)
        log.info('Killing DataMgr service')
        sudo('pkill -9 DataMgr')
        time.sleep(7)
        dm_pid_after = sudo('pgrep DataMgr')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                #if eachnode.services['DM'][0].status ==  'ACTIVE':
                if dm_pid_before ==  dm_pid_after:
                    log.warn('FAIL - Failed to kill DataMgr service on node {}'.format(eachnode.address.ipv4address))
                    return False 

                else:
                    log.info('PASS - DataMgr service has been killed on node {}'.format(eachnode.address.ipv4address))
                    return True

    def add(self, node_ip):
        '''
        Add DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add DM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(DMService.add.__name__)
        log.info('Adding DM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="DM"
    	#newNodeService.type="FDSP_ACCESS_MGR"
        newNodeService.type="DM"
        #newNodeService.status="ACTIVE"
        #newNodeService.state="UP"
        newNodeService.status.state="NOT_RUNNING"

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.add_service(eachnode.id, newNodeService)

		time.sleep(7)
        env.host_string = node_ip

	    #check updated node state 
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    if eachnode.services['DM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added DM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add DM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add DM service to node {}'.format(eachnode.address.ipv4address))
                    return False

    def remove(self, node_ip):
        '''
        Remove DM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove DM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(DMService.remove.__name__)
        log.info('Removing DM service')
        nodeNewState = Node()
        nodeNewState.dm=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['DM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                         log.warn('WARNING - DM service is not available on node {}'.format(eachnode.address.ipv4address))
                         return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['DM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['DM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['DM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove DM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed DM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - DM service has not been removed - unable to locate node')
            return False

class SMService(object):
    def __init__(self, om_ip, node_ip):
        #self.log_dir = fdscfg.rt_env.get_log_dir()
        #self.om_node =  fdscfg.rt_om_node
        #self.om_ip = self.om_node.nd_conf_dict['ip']
        self.om_ip =  om_ip
        self.node_ip =  node_ip
        env.user='root'
        env.password='passwd'
        env.host_string = self.om_ip
        TestUtils.create_fdsConf_file(self.om_ip)
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()

    def start(self, node_ip):
        '''
        Start SM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start SM service.

	    Returns:
	    -----------
	    oolean
        '''
        log.info(SMService.start.__name__)
        nodeNewState = Node()
        nodeNewState.sm=True

        #start SM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.start_service(eachnode.id, eachnode.services['SM'][0].id)
                time.sleep(7)
                env.host_string = node_ip
                sm_pid = sudo('pgrep StorMgr')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    if (eachnode.services['SM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - SM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - SM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = SM service has not started  - unable to locate node.')
            return False
                

    def stop(self, node_ip):
        '''
        Stop SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop SM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(SMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.sm=False
        NodeFound = False

        #stop SM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['SM'][0].id)
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['SM'][0].status.state ==  'INACTIVE' or eachnode.services['SM'][0].status.state ==  'DOWN' or eachnode.services['SM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - SM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - SM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                             log.warn('WARNING - SM service is not available on node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('Fail - SM service has not started - unable to locate node')
            return False

    def kill(self, node_ip):
        '''
        Kill SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill SM service.

    	Returns:
    	-----------
    	Boolean
        '''
        env.host_string = node_ip
        sm_pid_before = sudo('pgrep StorMgr')

        log.info(SMService.kill.__name__)
        log.info('Killing StorMgr service')
        sudo('pkill -9 StorMgr')
        time.sleep(7)
        sm_pid_after = sudo('pgrep StorMgr')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                if sm_pid_before ==  sm_pid_after:
                    log.warn('FAIL - Failed to kill StorMgr service on node {}'.format(eachnode.address.ipv4address))
                    return False 

                else:
                    log.info('PASS - StorMgr service has been killed on node {}'.format(eachnode.address.ipv4address))
                    return True

    def add(self, node_ip):
        '''
        Add SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add SM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(SMService.add.__name__)
        log.info('Adding SM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="SM"
        newNodeService.type="SM"
        #newNodeService.status="ACTIVE"
        #newNodeService.state="UP"
        newNodeService.status.state="NOT_RUNNING"

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.add_service(eachnode.id, newNodeService)

		time.sleep(7)
        env.host_string = node_ip

	    #check updated node state 
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    if eachnode.services['SM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added SM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add SM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add SM service to node {}'.format(eachnode.address.ipv4address))
                    return False

    def remove(self, node_ip):
        '''
        Remove SM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove SM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(SMService.remove.__name__)
        log.info('Removing SM service')
        nodeNewState = Node()
        nodeNewState.sm=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['SM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                         log.warn('WARNING - SM service is not available on node {}'.format(eachnode.address.ipv4address))
                         return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['SM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['SM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['SM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove SM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed SM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - SM service has not been removed - unable to locate node')
            return False

class PMService(object):
    def __init__(self, om_ip, node_ip):
        #self.log_dir = fdscfg.rt_env.get_log_dir()
        #self.om_node =  fdscfg.rt_om_node
        #self.om_ip = self.om_node.nd_conf_dict['ip']
        self.om_ip =  om_ip
        self.node_ip =  node_ip
        env.user='root'
        env.password='passwd'
        env.host_string = self.om_ip
        TestUtils.create_fdsConf_file(self.om_ip)
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()

    def start(self, node_ip):
        '''
        Start PM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start PM service.

	    Returns:
	    -----------
	    oolean
        '''
        log.info(PMService.start.__name__)
        nodeNewState = Node()
        nodeNewState.pm=True

        #start PM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                env.host_string = node_ip
                sudo('service fds-pm start')
                time.sleep(7)
                pm_pid = sudo('pgrep platformd')

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    if (eachnode.services['PM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - PM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - PM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = PM service has not started  - unable to locate node.')
            return False
                

    def stop(self, node_ip):
        '''
        Stop PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop PM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(PMService.stop.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.pm=False
        NodeFound = False

        #stop PM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['PM'][0].id)
                sudo('service fds-pm stop')
                time.sleep(7)
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['PM'][0].status.state ==  'STANDBY' or eachnode.services['PM'][0].status.state ==  'DOWN' or eachnode.services['PM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - PM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - PM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                             log.warn('WARNING - PM service is not available on node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('Fail - PM service has not started - unable to locate node')
            return False

    def kill(self, node_ip):
        '''
        Kill PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill PM service.

    	Returns:
    	-----------
    	Boolean
        '''
        env.host_string = node_ip
        pm_pid_before = sudo('pgrep platformd')

        log.info(PMService.kill.__name__)
        log.info('Killing platformd service')
        sudo('pkill -9 platformd')
        time.sleep(7)
        pm_pid_after = sudo('pgrep platformd')
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                if pm_pid_before ==  pm_pid_after:
                    log.warn('FAIL - Failed to kill platformd service on node {}'.format(eachnode.address.ipv4address))
                    return False 

                else:
                    log.info('PASS - platformd service has been killed on node {}'.format(eachnode.address.ipv4address))
                    return True

    def add(self, node_ip):
        '''
        Add PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add PM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(PMService.add.__name__)
        log.info('Adding PM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="PM"
        newNodeService.type="PM"
        #newNodeService.status="ACTIVE"
        #newNodeService.state="UP"
        newNodeService.status.state="NOT_RUNNING"

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.add_service(eachnode.id, newNodeService)

		time.sleep(7)
        env.host_string = node_ip

	    #check updated node state 
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    if eachnode.services['PM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added PM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add PM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add PM service to node {}'.format(eachnode.address.ipv4address))
                    return False

    def remove(self, node_ip):
        '''
        Remove PM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove PM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(PMService.remove.__name__)
        log.info('Removing PM service')
        nodeNewState = Node()
        nodeNewState.pm=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['PM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                         log.warn('WARNING - PM service is not available on node {}'.format(eachnode.address.ipv4address))
                         return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['PM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['PM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['PM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove PM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed PM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - PM service has not been removed - unable to locate node')
            return False

class OMService(object):
    def __init__(self, om_ip, node_ip):
        #self.log_dir = fdscfg.rt_env.get_log_dir()
        #self.om_node =  fdscfg.rt_om_node
        #self.om_ip = self.om_node.nd_conf_dict['ip']
        self.om_ip =  om_ip
        self.node_ip =  node_ip
        env.user='root'
        env.password='passwd'
        env.host_string = self.om_ip
        TestUtils.create_fdsConf_file(self.om_ip)
        fdsauth = FdsAuth()
        fdsauth.login()
        self.nservice = NodeService(fdsauth)
        self.node_list = self.nservice.list_nodes()

    def start(self, node_ip):
        '''
        Start OM service

	    Attributes:
	    -----------
	    node_ip:  str
		The IP address of the node to start OM service.

	    Returns:
	    -----------
	    oolean
        '''
        log.info(OMService.start.__name__)
        env.host_string = node_ip
        nodeNewState = Node()
        nodeNewState.om=True

        #start OM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                env.host_string = node_ip
                sudo('service fds-om start')
                time.sleep(7)
                om_pid = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")

        #check updated node state 
        node_list = self.nservice.list_nodes()
        try:
            for eachnode in node_list:
                if eachnode.address.ipv4address== node_ip:
                    if (eachnode.services['OM'][0].status.state ==  'RUNNING'):
                         log.info('PASS - OM service has started on node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - OM service has NOT started on node {}'.format(eachnode.address.ipv4address))
                         return False 

        except NameError:
            log.warn('FAIL = OM service has not started  - unable to locate node.')
            return False
                

    def stop(self, node_ip):
        '''
        Stop OM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to stop OM service.

    	Returns:
    	-----------
    	Boolean

        '''
        log.info(OMService.stop.__name__)
        env.host_string = node_ip
        om_pid_before = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")
        nodeNewState = Node()
        nodeNewState.om=False
        NodeFound = False

        #stop OM service
        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.stop_service(eachnode.id, eachnode.services['OM'][0].id)
                sudo('service fds-om stop')
                time.sleep(7)
                om_pid_after = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")
                NodeFound = True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['OM'][0].status.state ==  'INACTIVE' or eachnode.services['OM'][0].status.state ==  'DOWN' or eachnode.services['OM'][0].status.state ==  'NOT_RUNNING':
                             log.info('PASS - OM service is no longer running on node {}'.format(eachnode.address.ipv4address))
                             return True

                        else:
                             log.warn('FAIL - OM service is STILL running on node {}'.format(eachnode.address.ipv4address))
                             return False

                    except IndexError:
                             log.warn('WARNING - OM service is not available on node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('Fail - OM service has not started - unable to locate node')
            return False

    def kill(self, node_ip):
        '''
        Kill OM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to kill OM service.

    	Returns:
    	-----------
    	Boolean
        '''
        env.host_string = node_ip
        om_pid_before = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")

        log.info(OMService.kill.__name__)
        log.info('Killing om.formation.om.Main service')
        sudo('pkill -9 {}'.format(om_pid_before))
        time.sleep(7)
        om_pid_after = sudo("ps aux | grep om.Main | grep -v grep | awk '{print $2}'")
        #Get new node state
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                if om_pid_before == om_pid_after:
                    log.warn('FAIL - Failed to kill OM service on node {}'.format(eachnode.address.ipv4address))
                    return False 

                else:
                    log.info('PASS - OM service has been killed on node {}'.format(eachnode.address.ipv4address))
                    return True

    def add(self, node_ip):
        '''
        Add OM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to add OM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(OMService.add.__name__)
        log.info('Adding OM service')
        fdsauth2 = FdsAuth()
        fdsauth2.login()
        newNodeService = Service()
        newNodeService.auto_name="OM"
        newNodeService.type="OM"
        #newNodeService.status="ACTIVE"
        #newNodeService.state="UP"
        newNodeService.status.state="NOT_RUNNING"

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                self.nservice.add_service(eachnode.id, newNodeService)

		time.sleep(7)
        env.host_string = node_ip

	    #check updated node state 
        node_list = self.nservice.list_nodes()
        for eachnode in node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    if eachnode.services['OM'][0].status.state ==  'NOT_RUNNING':
                         log.info('PASS - Added OM service to node {}'.format(eachnode.address.ipv4address))
                         return True

                    else:
                         log.warn('FAIL - Failed to add OM service to node {}'.format(eachnode.address.ipv4address))
                         return False 

                except IndexError:
                    log.warn('FAIL - Failed to add OM service to node {}'.format(eachnode.address.ipv4address))
                    return False

    def remove(self, node_ip):
        '''
        Remove OM service

    	Attributes:
    	-----------
    	node_ip:  str
		The IP address of the node to remove OM service.

    	Returns:
    	-----------
    	Boolean
        '''
        log.info(OMService.remove.__name__)
        log.info('Removing OM service')
        nodeNewState = Node()
        nodeNewState.om=False
        NodeFound = False

        for eachnode in self.node_list:
            if eachnode.address.ipv4address == node_ip:
                try:
                    self.nservice.remove_service(eachnode.id, eachnode.services['OM'][0].id)
                    time.sleep(7)
                    NodeFound = True

                except IndexError:
                         log.warn('WARNING - OM service is not available on node {}'.format(eachnode.address.ipv4address))
                         return True

        #check updated node state 
        if NodeFound:
            node_list = self.nservice.list_nodes()
            for eachnode in node_list:
                if eachnode.address.ipv4address == node_ip:
                    try:
                        if eachnode.services['OM'][0].status.state ==  'INACTIVE' or \
                            eachnode.services['OM'][0].status.state ==  'NOT_RUNNING' or \
                            eachnode.services['OM'][0].status.state == 'RUNNING':
                             log.warn('FAIL - Failed to remove OM service from node {}'.format(eachnode.address.ipv4address))
                             return False 

                    except IndexError:
                             log.info('PASS - Removed OM service from node {}'.format(eachnode.address.ipv4address))
                             return True

        else:
            log.warn('FAIL - OM service has not been removed - unable to locate node')
            return False
