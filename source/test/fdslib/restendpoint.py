'''
Rest endpoints for use in python scripts
'''

import sys
import json
import unittest
import random
import xml.etree.ElementTree as ET
import time
from tabulate import tabulate
from collections import OrderedDict
import socket, struct
import pyfdsp.common.constants as TConstants

try:
    import requests
    from requests import ConnectionError
except ImportError:
    print 'oops! I need the [requests] pkg. do: "sudo easy_install requests" OR "pip install requests"'
    sys.exit(0)

import logging
logging.getLogger("requests").setLevel(logging.WARNING)
'''
try:
    import http.client as http_client
except ImportError:
    # Python 2
    import httplib as http_client
http_client.HTTPConnection.debuglevel = 1
requests_log = logging.getLogger("requests.packages.urllib3")
requests_log.setLevel(logging.DEBUG)
requests_log.propagate = True
'''
#----------------------------------------------------------------------------------------#
class RestException(Exception):
    pass

class UserException(RestException):
    pass

#----------------------------------------------------------------------------------------#

class RestEndpoint(object):

    def __init__(self, host='localhost', port=7443, auth=True, user='admin', password='admin', ssl=True):
        self.host = host
        self.port = port

        self.ssl = ssl
        self.setHost(host)
        self.setPort(port)
        self.setConfigPrefix()

        self.user = user
        self.password = password

        self._token = None
        self.headers = {}
        self.headers['prem'] = 'name'
        
        if auth:
            if not self.login(self.user, self.password):
                print '[WARN] : unable to login as {}'.format(self.user)

    def setHost(self, host):
        self.host = host
        self.base_path = 'http{}://{}:{}'.format('s' if self.ssl else '' , self.host,self.port)

    def setPort(self, port):
        self.port = port
        self.base_path = 'http{}://{}:{}'.format('s' if self.ssl else '',self.host,self.port)

    def setConfigPrefix(self):
        self.configPrefix = "/fds/config/" + TConstants.CURRENT_XDI_VERSION

    def login(self, user, password):
        '''
        Get the auth token from the auth REST endpoint.
        '''
        # TODO(brian): This will have to change when the token acquisition changes
        path = '{}/{}'.format(self.base_path, 'api/auth/token?login={}&password={}'.format(user, password))
        try :
            res = requests.get(path, verify=False)
            res = self.parse_result(res)
            self.user = user
            self.password = password
            self._token = res['token']
            self.headers = {'FDS-Auth' : self._token}
            return self._token
        except Exception as e:
            print e
            return None

    def get(self, path):
        #print("GET {}".format(path))
        return requests.get(path, headers=self.headers, verify=False)

    def post(self, path, data=None):
        #print("POST {}".format(path))
        return requests.post(path, headers=self.headers, data=data,  verify=False)

    def put(self, path, data=None):
        #print("PUT {}".format(path))
        return requests.put(path, headers=self.headers, data=data,  verify=False)

    def delete(self, path):
        #print("DELETE {}".format(path))
        return requests.delete(path, headers=self.headers,  verify=False)

    def parse_result(self, result):
        '''
        Parse a requests result and return the result as a dictionary
        Params:
           result - str : the JSON data to parse
        Returns:
           dictionary of key : value pairs parsed from JSON, None on failure
        '''

        if result.status_code == 200:
            try:
                return json.loads(result.text)
            except ValueError:
                print "JSON not valid!"
                return None
        else:
            print 'Non 200 response hit! ' + result.text
            return None


    # TODO(brian): add rest URL builder
    def build_path(self):
        pass

class ServiceEndpoint:

    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + '/api/config/nodes'

    def toggleServices(self, node_uuid, service_map):
        path = '{}/{}/1'.format(self.rest_path, str(node_uuid))
        res = self.rest.post(path, data=json.dumps(service_map))
        res = self.rest.parse_result(res)

    def listServices(self):
        nodes = self.listNodes()
        if not nodes:
            return []

        services = []
        for node in nodes:
            node_ip = node['ipV4address']
            for svc_name, svc_instances in node['services'].items():
                for svc_attrs in svc_instances:
                    service = {'port':      svc_attrs['port'],
                               'ip':        node_ip,
                               'service':   svc_name,
                               'status':    svc_attrs['status'],
                               'uuid':      svc_attrs['uuid']}
                    services.append(service)
        return services

    def listNodes(self):
        '''
        Get a list of nodes in the system.
        Params:
           None
        Returns:
           List of nodes
        '''

        res = self.rest.get(self.rest_path)
        res = self.rest.parse_result(res)
        if res is not None:
            return res['nodes']
        else:
            return []

    def startService(self, nodeUuid, svcs):
        rest_path = '{}/{}/1'.format(self.rest_path, nodeUuid)

        res = self.rest.post(rest_path, data=json.dumps(svcs))
        if res is not None:
            if res.content == '{"status": 0}\n':
                return True

        return False


class TenantEndpoint():
    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + '/api/system/tenants'

    def listTenants(self):
        '''
        List all tenants in the system.
        Params:
           None
        Returns:
           A list of dictionaries in the form:
           {
             'name' : value,
             'id' : value,
             'users' : [{
                 'identifier': value
                 'id': value
                 }]
           }
           or None on failure. Note: an empty dict {} means
           the call was successful, but no tenants were found.
        '''
        res = self.rest.get(self.rest_path)
        res = self.rest.parse_result(res)
        if res is not None:
            return res
        else:
            return None

    def createTenant(self, tenant_name):
        '''
        Create a new tenant in the system.
        Params:
           tenant_name - str: name of the new tenant to create
        Returns:
           integer id of the newly created tenant, None on failure
        '''

        path = '{}/{}'.format(self.rest_path, tenant_name)
        res = self.rest.post(path)
        res = self.rest.parse_result(res)
        if res is not None:
            return int(res['id'])
        else:
            return None

    def assignUserToTenant(self, user_id, tenant_id):
        '''
        Assign a user to a particular tenant.
        Params:
           user_id - int: Id of the user to assign to the tenant
           tenant_id - int: id of the tenant to assign the user to
        Returns:
           True on success, False otherwise
        '''
        path = '{}/{}/{}'.format(self.rest_path, tenant_id, user_id)
        res = self.rest.put(path)
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False


class VolumeEndpoint:

    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + '/fds/config/v08/volumes'

    def createVolume(self, name, priority, sla, limit, vol_type, size=10*1024, unit="MB", 
                    media_policy='hdd', commit_log_retention=86400, max_object_size=0):

        volume={u'accessPolicy': {u'exclusiveRead': True, u'exclusiveWrite': True},
                u'application': u'Application',
                u'created': {u'nanos': 0, u'seconds': u'0'},
                u'dataProtectionPolicy': {u'commitLogRetention': {u'nanos': 0,
                                                                  u'seconds': u'86400'},
                                          u'snapshotPolicies': []},
                u'mediaPolicy': u'HYBRID',
                u'name': u'prem',
                u'qosPolicy': {u'iopsMax': 0, u'iopsMin': 0, u'priority': 10},
                u'settings': {u'maxObjectSize': {u'unit': u'B', u'value': 2097152},
                              u'type': u'OBJECT'},
                u'uid': u'3' }

        volume['uid'] = 0
        volume['name'] = name
        volume['settings']['type'] = vol_type.upper()
        volume['settings']['maxObjectSize']['unit'] = 'B'
        volume['settings']['maxObjectSize']['value'] = max_object_size
        volume['dataProtectionPolicy']['commitLogRetention']['seconds'] = commit_log_retention

        res = self.rest.post(self.rest_path, data=json.dumps(volume))
        res = self.rest.parse_result(res)
        return res

    def deleteVolume(self, volume_name):
        path = '{}/{}'.format(self.rest_path, volume_name)

        res = self.rest.delete(path, headers=self.headers)
        res = self.rest.parse_result(res)


    def listVolumes(self):
        '''
        Get a list of volumes in the system.
        Params:
           None
        Returns:
           List of dictionaries describing volumes. Dictionaries in the form:
           {
              'name': <name>,
              'apis': <api>,
              'data_connector': { 'type': <type> },
              'id': <vol_id>,
              'priority': <priority>,
              'limit': <limit>,
              'sla': <sla>,
              'current_usage': {'unit': <unit>, 'size': <size> }
           }
        '''

        res = self.rest.get(self.rest_path)
        res = self.rest.parse_result(res)
        if res is not None:
            return res
        else:
            return None

    def setVolumeParams(self, vol_id, min_iops, priority, max_iops, media_policy, commit_log_retention):
        '''
        Set the QOS parameters for a volume.
        Params:
           vold_id - int: uuid of the volume to modify
           min_iops - int: minimum number of iops that must be sustained
           priority - int: priority of the volume
           max_iops - int: maximum number of iops that a volume may achieve
           media_policy - string: media policy
           commit_log_retention - int: commit log retention
        Returns:
           Dictionary of volume information including updated QOS params
           in the form:
           {
              'name': <name>,
              'apis': <apis>,
              'data_connector': {'type': <type>},
              'id': <vol_id>,
              'priority': <priority>,
              'limit': <limit>,
              'sla': <sla>,
              'current_usage': {'unit': <unit>, 'size': <size>}
           }
           or None on failure
        '''
        request_body = {
            "sla" : min_iops,
            "mediaPolicy" : media_policy,
            "priority" : priority,
            "commit_log_retention" : commit_log_retention,
            "limit" : max_iops,
        }

        path = '{}/{}'.format(self.rest_path, str(vol_id))
        res = self.rest.put(path, data=json.dumps(request_body))
        res = self.rest.parse_result(res)
        if res is not None:
            return res
        else:
            return None

    def modify(self, volume):
        path = '{}/{}'.format(self.rest_path, str(volume['uid']))
        res = self.rest.put(path, data=json.dumps(volume))
        res = self.rest.parse_result(res)
        if res is not None:
            return res
        else:
            return None


class AuthEndpoint:
    '''
    Authentication endpoints.
    '''
    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + '/api/auth'

    def grantToken(self):
        '''
        Get a token from the system. This token is the token used in the FDS-Auth header.
        Params:
           None
        Returns:
           Auth token as a string, None on failure
        '''
        res = self.rest.get('{}/{}'.format(self.rest_path, 'token?login=admin&password=admin'))
        res = self.rest.parse_result(res)
        if 'token' in res.keys():
            return res['token']
        else:
            return None

    def getCurrentUser(self):
        '''
        Get the current user's name/id.
        Params:
           None
        Returns:
           Dictionary of the form:
           {
              'identifier' : <username>,
              'id' : <user id>
              'isFdsAdmin' : <bool>
           }
           OR None on failure
        '''

        path = '{}/{}'.format(self.rest_path, 'currentUser')
        res = self.rest.get(path)
        res = self.rest.parse_result(res)
        if res != {}:
            return res
        else:
            return None


class UserEndpoint():

    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + '/api/system/users'

    def createUser(self, username, password):

        '''
        Create a new user in the system.
        Params:
           username - str: username of the new user
           password - str: password of the new user
        Returns:
           an integer id of the new user, None on failure
        '''
        path = '{}/{}/{}'.format(self.rest_path, username, password)

        res = self.rest.post(path)
        res = self.rest.parse_result(res)
        if res is not None:
            return int(res['id'])
        else:
            return None

    def listUsers(self):
        '''
        List all users in the system.
        Params:
           None
        Returns:
           List of users and their capabilities, None on failure
        '''
        res = self.rest.get(self.rest_path)
        res = self.rest.parse_result(res)
        if res is not None:
            return res
        else:
            return None

    def updatePassword(self, user_id, password):
        '''
        Update the password of a particular user.
        Params:
           user_id - int: id of the user to modify password for
           password - str: new password
        Returns:
           True success, False otherwise
        '''
        path = '{}/{}/{}'.format(self.rest_path, user_id, password)
        res = self.rest.put(path)
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

class S3Endpoint():

    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path

    def listBuckets(self, parse=True):
        try:
            r = self.rest.get(self.rest_path)
            #return r.text
            if parse:
                ns='{http://s3.amazonaws.com/doc/2006-03-01/}'
                root = ET.fromstring(r.text)
                return [item.text for item in root.findall('{}Bucket/{}Name'.format(ns,ns))]
            else:
                return r
        except ConnectionError:
            print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)
        except KeyboardInterrupt:
            print "request cancelled on interrupt"
        return None

    def bucketItems(self,bucket,parse=True):
        try:
            r = self.rest.get('{}/{}'.format(self.rest_path,bucket))
            if parse:
                ns='{http://s3.amazonaws.com/doc/2006-03-01/}'
                root = ET.fromstring(r.text)
                return [item.text for item in root.findall('%sContents/%sKey' % (ns,ns))]
            else:
                return r
        except ConnectionError:
            print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)
        except KeyboardInterrupt:
            print "request cancelled on interrupt"

    def createBucket(self,bucket):
        try:
            return self.rest.put('{}/{}'.format(self.rest_path,bucket))
        except ConnectionError:
            print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)
        except KeyboardInterrupt:
            print "request cancelled on interrupt"

    def deleteBucket(self,bucket):
        try:
            return self.rest.delete('{}/{}'.format(self.rest_path,bucket))
        except ConnectionError:
            print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)
        except KeyboardInterrupt:
            print "request cancelled on interrupt"

    def put(self,bucket,objname,data=None):
        try:
            r = self.rest.put('{}/{}/{}'.format(self.rest_path,bucket,objname),data=data)
            return r
        except ConnectionError:
            print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)

    def get(self,bucket,objname):
        try:
            r = self.rest.get('{}/{}/{}'.format(self.rest_path,bucket,objname))
            return r
        except ConnectionError:
            print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)
        except KeyboardInterrupt:
            print "request cancelled on interrupt"

    def delete(self,bucket,objname):
        try:
            return self.rest.delete('{}/{}/{}'.format(self.rest_path,bucket,objname))
        except ConnectionError:
                print "unable to connect to [%s:%s]"  % (self.rest.host,self.rest.port)
        except KeyboardInterrupt:
            print "request cancelled on interrupt"


class DomainEndpoint():

    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + '/local_domains'

    def createLocalDomain(self, domain_name, domain_site):

        '''
        Create a new local domain in the system.
        Params:
           domain_name - str: name of the new local domain
           domain_site - str: location of the new local domain
        Returns:
           the integer id of the new local domain, None on failure
        '''
        path = '{}/{}'.format(self.rest_path, domain_name)

        domain_info = {
            'site': domain_site
        }

        res = self.rest.post(path, data=json.dumps(domain_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if ('domainId' in res) and (int(res['domainId']) > 0):
                return int(res['domainId'])
            else:
                return None
        else:
            return None

    def listLocalDomains(self):
        '''
        List all local domains in the global domain.
        Params:
           None
        Returns:
           List of domains and their IDs, None on failure
        '''
        res = self.rest.get(self.rest_path)
        res = self.rest.parse_result(res)
        if res is not None:
            # Take the opportunity here to create the column headings
            # and ordering that we want.
            new_res = list()
            for row in res:
                new_row = OrderedDict([
                    ("name", row["name"]),
                    ("id", row["id"]),
                    ("site", row["site"])])
                new_res.append(new_row)

            return tabulate(new_res, headers="keys")
        else:
            return None

    def updateLocalDomainName(self, old_domain_name, new_domain_name):
        '''
        Change the domain_name of a particular local domain.
        Params:
           old_domain_name - str: Name of the local domain whose name is to be changed
           new_domain_name - str: New name of the local domain
        Returns:
           True success, False otherwise
        '''

        path = '{}/{}'.format(self.rest_path, old_domain_name)

        domain_info = {
            'action': 'rename',
            'new_domain_name': new_domain_name,
        }

        res = self.rest.put(path, data=json.dumps(domain_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def updateLocalDomainSite(self, domain_name, new_site_name):
        '''
        Change the site_name of a particular local domain.
        Params:
           domain_name - str: Name of the local domain whose site is to be changed
           new_site_name - str: New name of the local domain site
        Returns:
           True success, False otherwise
        '''

        path = '{}/{}'.format(self.rest_path, domain_name)

        domain_info = {
            'action': 'change_site',
            'new_site_name': new_site_name,
        }

        res = self.rest.put(path, data=json.dumps(domain_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def setThrottle(self, domain_name, throttle_level):
        '''
        Set the throttle level on the specified local domain.
        Params:
           domain_name - str: Name of the local domain whose throttle level is to be set
           throttle_level - float: ?
        Returns:
           True success, False otherwise
        '''

        path = '{}/{}/throttle'.format(self.rest_path, domain_name)

        throttle_info = {
            'throttle_level': throttle_level,
        }

        res = self.rest.put(path, data=json.dumps(throttle_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def setScavenger(self, domain_name, scavenger_action):
        '''
        Set the Scavenger action on the specified local domain.
        Params:
           domain_name - str: Name of the local domain whose Scavenger action is to be set
           scavenger_action - str: "enable", "disable", "start", "stop"
        Returns:
           True success, False otherwise
        '''

        path = '{}/{}/scavenger'.format(self.rest_path, domain_name)

        scavenger_info = {
            'action': scavenger_action,
        }

        res = self.rest.put(path, data=json.dumps(scavenger_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def startupLocalDomain(self, domain_name):
        '''
        Startuo the specified local domain.
        Params:
           domain_name - str: Name of the local domain to be started
        Returns:
           True success, False otherwise
        '''

        path = '{}/{}'.format(self.rest_path, domain_name)

        domain_info = {
            'action': 'startup',
            }

        res = self.rest.put(path, data=json.dumps(domain_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def shutdownLocalDomain(self, domain_name):
        '''
        Shutdown the specified local domain.
        Params:
           domain_name - str: Name of the local domain to be shutdown
        Returns:
           True success, False otherwise
        '''

        path = '{}/{}'.format(self.rest_path, domain_name)

        domain_info = {
            'action': 'shutdown',
        }

        res = self.rest.put(path, data=json.dumps(domain_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def deleteLocalDomain(self, domain_name):
        '''
        Delete the specified local domain.
        Params:
           domain_name - str: Name of the local domain to be deleted
        Returns:
           True success, False otherwise
        '''
        path = '{}/{}'.format(self.rest_path, domain_name)
        res = self.rest.delete(path)
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False


    def activateLocalDomainServices(self, domain_name, sm, dm, am):
        '''
        Activate the specified or pre-defined services on all the nodes in the specified local domain. If
        all Service switches are false, we interpret it as meaning to activate all defined
        Services for each node. If in that case there are no Services defined for the Node, we interpret
        it to mean activate all Services (SM, DM,and AM) for the Node.

        Params:
           domain_name - str: Name of the local domain whose node services are to be activated
           sm - bool: Activate the SM Service (True)
           dm - bool: Activate the DM Service (True)
           am - bool: Activate the AM Service (True)

        Returns:
           True success, False otherwise
        '''

        path = '{}/{}/services'.format(self.rest_path, domain_name)

        service_info = {
            'action': 'activate',
            'sm': sm,
            'dm': dm,
            'am': am,
        }

        res = self.rest.put(path, data=json.dumps(service_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False

    def listLocalDomainServices(self, domain_name):
        '''
        List all services that reside within the local domain.
        Params:
           domain_name - str: Name of the local domain whose services are to be listed
        Returns:
           List of services that reside within the specified local domain, None on failure
        '''
        path = '{}/{}/services'.format(self.rest_path, domain_name)
        res = self.rest.get(path)
        res = self.rest.parse_result(res)
        if res is not None:
            # Take the opportunity here to create the column headings
            # and ordering that we want.
            new_res = list()
            for row in res:
                new_row = OrderedDict([
                    ("node_uuid", "0x"+hex(row["node_uuid"])[2:].zfill(16)),
                    ("node_root", row["node_root"]),
                    ("node_id", row["node_id"]),
                    ("IPv4", socket.inet_ntoa(struct.pack('!L', row["ip_lo_addr"]))),
                    ("IPv6", socket.inet_ntoa(struct.pack('!L', row["ip_hi_addr"]))),
                    ("service_name", row["node_name"]),
                    ("service_type", row["node_type"]),
                    ("service_uuid", "0x"+hex(row["service_uuid"])[2:].zfill(16)),
                    ("service_state", row["node_state"]),
                    ("control_port", row["control_port"]),
                    ("data_port", row["data_port"]),
                    ("migration_port", row["migration_port"]),
                    ("metasync_port", row["metasync_port"])])
                new_res.append(new_row)

            return tabulate(new_res, headers="keys")
        else:
            return None

    def removeLocalDomainServices(self, domain_name, sm, dm, am):
        '''
        Remove the specified or pre-defined services on all the nodes in the specified local domain. If
        all Service switches are false, we interpret it as meaning to remove all defined
        Services for each node. If in that case there are no Services defined for the Node, we
        do nothing.

        Note that Service removal means to unregister the Service from the Local Domain and
        stop the processes associated with the Service.

        Params:
           domain_name - str: Name of the local domain whose node services are to be removed
           sm - bool: Remove the SM Service (True)
           dm - bool: Remove the DM Service (True)
           am - bool: Remove the AM Service (True)

        Returns:
           True success, False otherwise
        '''

        path = '{}/{}/services'.format(self.rest_path, domain_name)

        service_info = {
            'action': 'remove',
            'sm': sm,
            'dm': dm,
            'am': am,
        }

        res = self.rest.put(path, data=json.dumps(service_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False


class QoSPolicyEndpoint():

    def __init__(self, rest):
        self.rest = rest
        self.rest_path = self.rest.base_path + self.rest.configPrefix + '/qos_policies'

    def createQoSPolicy(self, policy_name, iops_min, iops_max, rel_prio):

        '''
        Create a new QoS Policy in the system.
        Params:
           policy_name - str: name of the new QoS Policy
           iops_min - int: minimum IOPS
           iops_max - int: maximum IOPS
           rel_prio - int: relative priority
        Returns:
           A JSON set of the newly created QoS Policy attributes.
        '''
        path = self.rest_path

        policy_info = {
            'policy_name': policy_name,
            'iops_min': iops_min,
            'iops_max': iops_max,
            'rel_prio': rel_prio
        }

        res = self.rest.post(path, data=json.dumps(policy_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if ('policy_id' in res) and (int(res['policy_id']) > 0):
                return res
            else:
                return None
        else:
            return None

    def listQoSPolicies(self):
        '''
        List all QoS Policies in the global domain.
        Params:
           None
        Returns:
           List of Qos Policies and their attributes, None on failure
        '''
        res = self.rest.get(self.rest_path)
        res = self.rest.parse_result(res)
        if res is not None:
            # Take the opportunity here to create the column headings
            # and ordering that we want.
            new_res = list()
            for row in res:
                new_row = OrderedDict([
                    ("policy_name", row["policy_name"]),
                    ("policy_id", row["policy_id"]),
                    ("iops_min", row["iops_min"]),
                    ("iops_max", row["iops_max"]),
                    ("rel_prio", row["rel_prio"])])
                new_res.append(new_row)

            return tabulate(new_res, headers="keys")
        else:
            return None

    def updateQosPolicy(self, current_policy_name, new_policy_name, iops_min, iops_max, rel_prio):
        '''
        Change the attributes of a particular QoS Policy.
        Params:
            current_policy_name - str: Current name of the QoS Policy.
            new_policy_name - str: New name for the QoS Policy. Same as current name if name is not changing.
            iops_min - int: New minimum IOPS guarantee.
            iops_max - int: New maximum IOPS guarantee.
            rel_prio - int: New relative priority.
        Returns:
           The updated policy results are returned upon success, otherwise nothing.
        '''

        path = '{}/{}'.format(self.rest_path, current_policy_name)

        domain_info = {
            'new_policy_name': new_policy_name,
            'iops_min': iops_min,
            'iops_max': iops_max,
            'rel_prio': rel_prio,
        }

        res = self.rest.put(path, data=json.dumps(domain_info))
        res = self.rest.parse_result(res)
        if res is not None:
            if ('policy_id' in res) and (int(res['policy_id']) > 0):
                return res
            else:
                return None
        else:
            return None

    def deleteQoSPolicy(self, policy_name):
        '''
        Delete the specified QoS Policy.
        Params:
           policy_name - str: Name of the QoS Policy to be deleted
        Returns:
           True success, False otherwise
        '''
        path = '{}/{}'.format(self.rest_path, policy_name)
        res = self.rest.delete(path)
        res = self.rest.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False


            
class TestServiceEndpoints(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        rest = RestEndpoint()
        self.svcEp = ServiceEndpoint(rest)

    def test_listVolume(self):
        vols = self.svcEp.listNodes()
        self.assertIsNotNone(vols)

    def test_activateNode(self):
        node_uuid = random.randint(0, 10000)
        status = self.svcEp.activateNode(node_uuid, {})
        self.assertEquals(status.lower(), 'ok')

    def test_deactivateNode(self):
        node_uuid = random.randint(0, 10000)
        status = self.svcEp.deactivateNode(node_uuid)
        self.assertEquals(status.lower(), 'ok')

class TestS3Endpoint(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        rest = RestEndpoint(port=8000, auth=False)
        self.bucket='testbbb'
        self.api = S3Endpoint(rest)
        self.api.createBucket(self.bucket)
        time.sleep(1)

    def test_listBuckets(self):
        r = self.api.createBucket(self.bucket)
        self.assertIn(r.status_code,[200,409])
        buckets = self.api.listBuckets()
        self.assertIn(self.bucket,buckets)

    def test_items(self):
        r = self.api.createBucket(self.bucket)
        self.assertIn(r.status_code,[200,409])
        r = self.api.put(self.bucket, 'a', 'b')
        self.assertEquals(r.status_code,200)

        items = self.api.bucketItems(self.bucket)
        self.assertIn('a', items)

        r = self.api.delete(self.bucket,'a')
        self.assertEquals(r.status_code,204)

        items = self.api.bucketItems(self.bucket)
        self.assertNotIn('a', items)

class TestVolumeEndpoints(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        rest = RestEndpoint()
        self.volEp = VolumeEndpoint(rest)

    def test_listVolume(self):
        vols = self.volEp.listVolumes()
        self.assertIsNotNone(vols)

    def test_createVolume(self):
        vol_name = 'unit_test' + str(random.randint(0, 10000))
        status = self.volEp.createVolume(vol_name, 1, 1, 5000, 'object', 5000, 'mb')
        self.assertEquals(status.lower(), 'ok')

    def test_setVolumeParameters(self):
        vol_name = 'unit_test' + str(random.randint(0, 10000))
        status = self.volEp.createVolume(vol_name, 1, 1, 5000, 'object', 5000, 'mb')
        self.assertEquals(status.lower(), 'ok')

        vols = self.volEp.listVolumes()
        vol_id = None
        for vol in vols:
            if vol['name'] == vol_name:
                vol_id = vol['id']

        res = self.volEp.setVolumeVolumeParams(vol_id, 50, 1, 100, 'HDD_ONLY', 86400)
        self.assertDictContainsSubset({'name': vol_name, 'id':vol_id}, res)

class TestTenantEndpoints(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        rest = RestEndpoint()
        self.tenantEp = TenantEndpoint(rest)
        self.userEp = UserEndpoint(rest)

    def test_assignUserToTenant(self):

        id1 = self.userEp.createUser('ten_user1', 'abc')
        self.assertGreater(id1, 1)

        tn1 = self.tenantEp.createTenant('foo')
        self.assertGreater(tn1, 0)

        res = self.tenantEp.assignUserToTenant(id1, tn1)
        self.assertTrue(res)

    def test_createTenant(self):
        id1 = self.tenantEp.createTenant('abc')
        self.assertGreater(id1, 0)



class TestUserEndpoints(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        rest = RestEndpoint()
        self.userEp = UserEndpoint(rest)

    def test_createUser(self):
        self.id1 = self.userEp.createUser('ut_user1', 'abc')
        self.assertGreater(self.id1, 1)
        self.id2 = self.userEp.createUser('ut_user2', 'abc')
        self.assertGreater(self.id2, self.id1)

    def test_listUsers(self):

        id1 = self.userEp.createUser('ut_user3', 'abc')
        id2 = self.userEp.createUser('ut_user4', 'abc')

        users = self.userEp.listUsers()
        self.assertIn({u'identifier' : u'admin', u'isFdsAdmin': True, u'id': 1}, users)
        self.assertIn({u'identifier' : u'ut_user3', u'isFdsAdmin': False, u'id': int(id1)}, users)
        self.assertIn({u'identifier' : u'ut_user4', u'isFdsAdmin': False, u'id': int(id2)}, users)


    def test_changePassword(self):

        id1 = self.userEp.createUser('ut_user5', 'abc')
        self.assertGreater(id1, 1)

        cp = self.userEp.updatePassword(id1, 'def')
        self.assertTrue(cp)

if __name__ == "__main__":
    unittest.main()
