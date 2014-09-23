'''
Rest endpoints for use in python scripts
'''

import sys
import json
import unittest
import random

try:
    import requests
except ImportError:
    print 'oops! I need the [requests] pkg. do: "sudo easy_install requests" OR "pip install requests"'
    sys.exit(0)

import logging
logging.getLogger("requests").setLevel(logging.WARNING)

#----------------------------------------------------------------------------------------#
class RestException(Exception):
    pass

class UserException(RestException):
    pass

#----------------------------------------------------------------------------------------#

class RestEndpoint(object):
    
    def __init__(self, host='localhost', port=7777, auth=True):
        self.host = host
        self.port = port

        self.setHost(host)
        self.setPort(port)
        
        self.user = 'admin'
        self.password = 'admin'
        self._token = None
        self.headers = []
        if auth:
            if not self.login(self.user, self.password):
                print '[WARN] : unable to login as {}'.format(self.user)

    def setHost(self, host):
        self.host = host
        self.base_path = 'http://{}:{}'.format(self.host,self.port)

    def setPort(self, port):
        self.port = port
        self.base_path = 'http://{}:{}'.format(self.host,self.port)

    def login(self, user, password):
        '''
        Get the auth token from the auth REST endpoint.
        '''
        # TODO(brian): This will have to change when the token acquisition changes
        path = '{}/{}'.format(self.base_path, 'api/auth/token?login={}&password={}'.format(user, password))
        try :
            res = requests.get(path)
            res = self.parse_result(res)
            self.user = user
            self.password = password
            self._token = res['token']
            self.headers = {'FDS-Auth' : self._token}
            return self._token
        except:
            return None

    def get(self, path):
        return requests.get(path, headers=self.headers)

    def post(self, path, data=None):
        return requests.post(path, headers=self.headers, data=data)

    def put(self, path, data=None):
        return requests.put(path, headers=self.headers, data=data)

    def delete(self, path):
        return requests.delete(path, headers=self.headers)

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
        self.rest_path = self.rest.base_path + '/api/config/volumes'

    def createVolume(self, volume_name, priority, sla, limit, vol_type, size, unit):

        volume_info = {
            'name' : volume_name,
            'priority' : int(priority),
            'sla': int(sla),
            'limit': int(limit),
            'data_connector': {
                'type': vol_type,
                'attributes': {
                    'size': size,
                    'unit': unit
                }
            }
        }
        res = self.rest.post(self.rest_path, data=json.dumps(volume_info))
        res = self.rest.parse_result(res)

        if type(res) != dict or 'status' not in res:
            return None
        else:
            if res['status'].lower() != 'ok':
                return None
            else:
                return res['status']
        
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
        
    def setVolumeQosParams(self, vol_id, min_iops, priority, max_iops):
        '''
        Set the QOS parameters for a volume.
        Params:
           vold_id - int: uuid of the volume to modify
           min_iops - int: minimum number of iops that must be sustained
           priority - int: priority of the volume
           max_iops - int: maximum number of iops that a volume may achieve
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

        params = {
            'sla' : min_iops,
            'priority': priority,
            'limit': max_iops
        }
        path = '{}/{}'.format(self.rest_path, str(vol_id))
        res = self.rest.put(path, data=json.dumps(params))
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

    def test_setQosParameters(self):
        vol_name = 'unit_test' + str(random.randint(0, 10000))
        status = self.volEp.createVolume(vol_name, 1, 1, 5000, 'object', 5000, 'mb')
        self.assertEquals(status.lower(), 'ok')

        vols = self.volEp.listVolumes()
        vol_id = None
        for vol in vols:
            if vol['name'] == vol_name:
                vol_id = vol['id']

        res = self.volEp.setVolumeQosParams(vol_id, 50, 1, 100)
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
