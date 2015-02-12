#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import paramiko


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
        self.ssh_conn = paramiko.SSHClient()
        self.ssh_conn.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh_conn.connect(hostname, username=user, password=pwd,
                              key_filename=filename)