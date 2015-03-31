#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import paramiko
import config

class SSHConn(object):
    '''
    Class SSHConn is a wrapper around paramiko's ssh client. In order to make
    things simpler for the user, we create a ssh_conn object for the SSHConn
    which can execute code remotely.

    Attributes:
    -----------
    hostname: str
        the ip address (or name) of the server which user wishes to connect.
    user : str
        the username
    pwd : str
        the password
    key_filename : str
        the name of the keyfile to be used
    '''
    def __init__(self, hostname, user, pwd, filename=None):
        self.client = paramiko.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.fname = None
        if filename is not None:
            fname = os.path.join(config.ANSIBLE_ROOT, filename)
        self.client.connect(hostname, username=user, password=pwd,
                              key_filename=fname)
        self.transport = self.client.get_transport()
        self.channel = self.transport.open_session()
