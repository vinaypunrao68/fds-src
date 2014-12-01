#!/usr/bin/env python
""" This script is used to export/import/list tenants and users in an FDS
system. It should eventually be replaced by fdsconsole but for now we need
a way to create a group of users & tenants for our Formation Cloud system. """
import os
import sys
import argparse
import requests
import inspect
import json

DEFAULT_URL = 'http://localhost:7777'
DEFAULT_USERNAME = 'admin'
DEFAULT_PASSWORD = 'admin'
DEFAULT_VERIFY = True

class FormationAuth:
    """Common class for authentication related activities"""

    def __init__(self, username=DEFAULT_USERNAME, password=DEFAULT_PASSWORD, url=DEFAULT_URL, verify=DEFAULT_VERIFY):
        self.username = username
        self.password = password
        self.url = url
        self.verify = verify
        self.token = self.get_token()
        self.r_headers = {'FDS-Auth': self.token}
        self.tenants = self.get_resource('tenants')
        self.users = self.get_resource('users')

    def try_req(self, req_type, req):
        """Wrap calls to requests with a try to catch any exceptions thrown
        and expose HTTP errors - This method accepts any request type that
        the requests module support passed in via 'req_type' """
        try:
            call = getattr(requests, req_type)
            r = call(req, headers=self.r_headers, verify=self.verify)
        except requests.exceptions.RequestException as e:
            print e
            sys.exit(1)
        return r

    def get_token(self):
        """Obtain the auth token from the FDS Api and store in headers which
        are re-used on each request"""
        self.r_headers = {'FDS-Auth': ''}
        req = '%s/api/auth/token?login=%s&password=%s' % (
                self.url,
                self.username,
                self.password)
        print("Getting credentials from: ", req)
        r = self.try_req('get', req)
        rjson = r.json()
        return rjson['token']

    def show_token(self):
        """Show token obtained via auth to assist in manual testing"""
        print "Token: %s" % self.token

    # Return list of specified resource (tenants or users)
    def get_resource(self, resource):
        """Generic requestor for a list of the specified resource type. Right
        now those resources can be 'tenants' or 'users'. This script doesn't
        make much use of the 'users' resource except to list it. All import
        and export activity utilizes the 'tenants' resource"""
        req = '%s/api/system/%s' % (self.url, resource)
        r = self.try_req('get', req)
        return r.json()

    def list_tenants(self):
        """List all tenants in the system and their associated users"""
        for tenant in self.tenants:
            print "Tenant: %s ID: %s" % (str(tenant['name']), str(tenant['id']))
            for user in tenant['users']:
                print "\tUser: %s ID: %s" % (user['identifier'], user['id'])

    def list_users(self):
        """List all users in the system and their associated tenant"""
        for user in self.users:
            if 'tenant' in user:
                tenant_name = user['tenant']['name']
            else:
                tenant_name = 'Unassigned'
            print "Name: %s ID: %s Admin: %s Tenant: %s" % (
                    user['identifier'],
                    str(user['id']),
                    user['isFdsAdmin'],
                    tenant_name)

    def get_tenant_by_id(self, id):
        """Obtain a tenant dict by ID"""
        for tenant in self.tenants:
            if tenant['id'] == id:
                return tenant

    def get_tenant_by_name(self, name):
        """Obtain tenant dict by name"""
        for tenant in self.tenants:
            if tenant['name'] == name:
                return tenant

    def get_user_by_id(self, id):
        """Obtain user dict by ID"""
        for user in self.users:
            if user['id'] == id:
                return user

    def get_user_by_name(self, name):
        """Obtain user dict by name"""
        for user in self.users:
            if user['identifier'] == name:
                return user

    def create_tenant(self, name):
        """Create a tenant by name - returns assigned ID"""
        req = '%s/api/system/tenants/%s' % (self.url, name)
        r = self.try_req('post', req)
        self.refresh_resources()
        tenant = self.get_tenant_by_name(name)
        print "Tenant Created -- Name: %s ID: %s" % (
                tenant['name'],
                tenant['id'])
        return tenant['id']

    def create_user(self, name, password):
        """Create a user by name & password"""
        req = '%s/api/system/users/%s/%s' % (self.url, name, password)
        r = self.try_req('post', req)
        self.refresh_resources()
        user = self.get_user_by_name(name)
        print "User Created -- Name: %s ID: %s" % (
                user['identifier'],
                user['id'])
        return user['id']

    def assign_user(self, user_id, tenant_id):
        """Assign a user to a tenant"""
        req = '%s/api/system/tenants/%s/%s' % (self.url, tenant_id, user_id)
        r = self.try_req('put', req)
        self.refresh_resources()

    def refresh_resources(self):
        self.tenants = self.get_resource('tenants')
        self.users = self.get_resource('users')

    def create_tenants_and_users(self, userlist, dryrun):
        """Create tenants and users from a list provided following the
        format provided by import. Intended to re-hydrate a list of
        tenants and users for a system from a json list created by
        the export method"""
        # For now we only worry about the tenant list
        for tenant in userlist['tenants']:
            if dryrun == True:
                print "[DRYRUN] Create tenant: %s" % tenant['name']
            else:
                tenant_id = self.create_tenant(tenant['name'])
            for user in tenant['users']:
                password = user['password'] if 'password' in user.keys() else 'testpass'
                if dryrun == True:
                    print "[DRYRUN] Create User: %s" % user['identifier']
                    print "[DRYRUN] Assign User %s to tenant %s" % (
                            user['identifier'], tenant['name'])
                else:
                    user_id = self.create_user(user['identifier'], password)
                    self.assign_user(user_id, tenant_id)

# End class FormationAuth

def setup_client(args):
    """Initialize the auth API client"""
    # Establish a client
    client = FormationAuth(username=args.username,
                           password=args.password,
                           url=args.url,
                           verify=args.verify)
    return client

def export_main(args, client):
    """Export list of tenants and users to a json file specified by
    args.filename suitable for importing using import
    called when the 'export' sub-command is specified. """
    tenants = client.get_resource('tenants')
    users = client.get_resource('users')
    userlist = {"tenants": tenants, "users": users}
    if args.dryrun == True:
        print "[DRYRUN] Would write the following to %s" % args.filename
        print userlist
    else:
        outfile = open(args.filename, 'w')
        output = json.dump(userlist, outfile, sort_keys=True, indent=4)
        outfile.close()

def import_main(args, client):
    """Import list of tenants and users from a json file specified by
    args.filename. Should be created by export function but supports an
    additional per-user parameter for password which may be added to the
    json. Called when the 'import' sub-command is specified. """
    infile = open(args.filename, 'r')
    userlist = json.load(infile)
    client.create_tenants_and_users(userlist, args.dryrun)

def list_main(args, client):
    """List tenants and users - called when the 'list' sub-command is
    specified. """
    client.list_tenants()
    client.list_users()

def main():
    """ Parse arguments & then call associated sub-command method. """
    parser = argparse.ArgumentParser(description='Show & Create Users and Tenants')
    parser.add_argument('-u', dest='url',
            default=DEFAULT_URL,
            help='URL for auth API (default: http://localhost:7777)')
    parser.add_argument('-U', dest='username',
            default=DEFAULT_USERNAME,
            help='API username (default: admin)')
    parser.add_argument('-P', dest='password',
            default=DEFAULT_PASSWORD,
            help='API password (default: admin)')
    parser.add_argument('-k', dest='verify',
            default=DEFAULT_VERIFY, action='store_false',
            help='Disable cert checking for SSL')
    parser.add_argument('-d', dest='dryrun',
            default=False, action='store_true',
            help='Print what will happen - dry run')

    subparsers = parser.add_subparsers(help='sub-command help')

    # export command
    parser_export = subparsers.add_parser('export', help='export tenants and users')
    parser_export.add_argument('-f', dest='filename', help='export to filename')
    parser_export.set_defaults(func=export_main)

    parser_import = subparsers.add_parser('import', help='import tenants and users')
    parser_import.add_argument('-f', dest='filename', help='import to filename')
    parser_import.set_defaults(func=import_main)

    parser_list = subparsers.add_parser('list', help='list tenants and users')
    parser_list.set_defaults(func=list_main)

    args = parser.parse_args()
    client = setup_client(args)
    client.show_token()
    args.func(args, client)


if __name__ == "__main__":
    main()
