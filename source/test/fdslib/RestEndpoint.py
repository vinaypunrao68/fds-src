'''
Rest endpoints for use in python scripts
'''

import sys
import re
import os
import json
import unittest

try:
    import requests
except ImportError:
    print 'oops! I need the [requests] pkg. do: "sudo easy_install requests" OR "pip install requests"'
    sys.exit(0)

#----------------------------------------------------------------------------------------#
class RestException(Exception):
    pass

class UserException(RestException):
    pass

#----------------------------------------------------------------------------------------#

class RestEndpoint(object):
    
    def __init__(self, host='localhost', port=7777, auth=True):

        if 'http://' not in host:
            host = 'http://' + host
            
        self.host = host
        self.port = port
        self.rest_path = host + ':' + str(port)

        if auth:
            self._token = self.__get_auth_token()
            self.headers = {'FDS-Auth' : self._token}

    def __get_auth_token(self):
        '''
        Get the auth token from the auth REST endpoint.
        '''
        # TODO(brian): This will have to change when the token acquisition changes
        path = '{}/{}'.format(self.rest_path, 'api/auth/token?login=admin&password=admin')

        res = requests.get(path)
        res = self.parse_result(res)
        return res['token']

    

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
            print result.text
            return None


    # TODO(brian): add rest URL builder
    def build_path(self):
        pass


class TenantEndpoint(RestEndpoint):
    def __init__(self, **kwargs):
        RestEndpoint.__init__(self, **kwargs)

        self.rest_path += '/api/system/tenants'

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
        res = requests.get(self.rest_path)
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
        res = requests.post(path, headers=self.headers)
        res = self.parse_result(res)
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
        res = requests.put(path, headers=self.headers)
        res = self.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False
    

class UserEndpoint(RestEndpoint):
    
    def __init__(self, **kwargs):
        RestEndpoint.__init__(self, **kwargs)
        self.rest_path += '/api/system/users'

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

        res = requests.post(path, headers=self.headers)
        res = self.parse_result(res)
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
        res = requests.get(self.rest_path, headers=self.headers)
        res = self.parse_result(res)
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
        res = requests.put(path, headers=self.headers)
        res = self.parse_result(res)
        if res is not None:
            if 'status' in res and res['status'].lower() == 'ok':
                return True
            else:
                return False
        else:
            return False


class TestTenantEndpoints(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.tenantEp = TenantEndpoint()
        self.userEp = UserEndpoint()
        
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
        self.userEp = UserEndpoint()

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
