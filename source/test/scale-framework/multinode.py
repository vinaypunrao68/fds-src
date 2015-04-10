#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import logging
import os
import shutil
import subprocess
import sys

from os.path import expanduser

import config

class Multinode(object):

    '''
    The class Multinode supports creating a Multinode FDS cluster.
    There are two supported instances: AWS and existing local clusters. If AWS
    is used, then the minimum value of the cluster is 4 nodes, and a name has
    to be given to the cluster.
    An existing cluster can be used, and its inventory file has to be provided. 
    Otherwise, the default 4 node cluster from the framework will be used.
    (see 'integration-framework-cluster' inventory in templates).

    Attributes:
    -----------
    name : str
        the name of the cluster (if AWS)
    instance_count : int
        how many aws instances will be created for the cluster.
    type: str
        could be 'aws' or 'baremetal'
    build: str
        specify if the build to be used is local or nightly.
    inventory:
        if 'baremetal' is used, the user could specify its own inventory file.
        Otherwise, the default 'baremetal' inventory file will be utilized.
    '''
    instance_types = ('aws', 'baremetal', 'static_aws')
    build_types = ("nightly", "local")
    baremetal_inventory = "scale-framework-cluster"
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)

    def __init__(self, name="", instance_count=1, type='baremetal',
                 build="nightly", inventory=None):
        self.instance_count = instance_count
        if type not in self.instance_types:
            raise ValueError, 'Invalid Instance type. The supported versions' \
                ' are %s' % self.instance_types
            sys.exit(2)
        self.instance_type = type
        self.static_aws_inventory = None
        self.cluster_name = name
        if build not in self.build_types:
            raise ValueError, 'Invalid build type. The supported versions ' \
                'are %s' % self.build_types
            sys.exit(2)
        self.build_type = build
        if self.__check_preconditions(inventory):
            self.start_cluster()
        else:
            self.destroy_cluster()

    def __check_preconditions(self, inventory):
        '''
        Check if all the pre-conditions for creating a multinode cluster is in
        place. If not, raise an exception highlighting the problem, and return
        the appropriate value.

        Attributes:
        inventory : str
            the name of the inventory file. If none or 'baremetal', then the
            'baremetal' provision will be used.
        Returns:
        --------
        boolean : the status of the preconditions.
        '''
        if self.instance_type == "aws":
            inventory_path = os.path.join(config.ANSIBLE_ROOT,
                                          "inventory/%s" % self.cluster_name)
            # check if the cluster name file exists
            if not os.path.exists(inventory_path):
                local_inventory = config.EC2_TEMPLATE
                if not os.path.exists(local_inventory):
                    raise OSError, "Template %s must exist" % local_inventory
                    return False
                shutil.copyfile(local_inventory, inventory_path)
            vault = os.path.join(expanduser("~"), config.VAULT_PASS)
            if not os.path.exists(vault):
                raise OSError, "Vault Pass file must exist before cluster is" \
                    " run."
                return False
            return True

        elif self.instance_type == "static_aws":
            if inventory is None:
                self.static_aws_inventory = config.DEFAULT_AWS_INVENTORY
            else:
                self.static_aws_inventory = inventory
            inventory_path = os.path.join(config.ANSIBLE_INVENTORY,
                                          self.static_aws_inventory)
            # check if the static aws inventory exists.
            if not os.path.exists(inventory_path):
                raise OSError, "Static AWS template %s must exist" % self.static_aws_inventory
                return False
            return True

        elif self.instance_type == "baremetal":
            if inventory is not None:
                self.baremetal_inventory = inventory
            inventory_path = os.path.join(config.ANSIBLE_ROOT,
                                          "inventory/%s" %
                                          self.baremetal_inventory)
            # check if the baremetal inventory file exists
            if not os.path.exists(inventory_path):
                local_inventory = config.BAREMETAL_TEMPLATE
                if not os.path.exists(local_inventory):
                    raise OSError, "Template %s must exist"  % local_inventory
                    return False
                shutil.copyfile(local_inventory, inventory_path)
            return True
        else:
            raise Exception, "Invalid instance type: %s" % self.instance_type

    def start_cluster(self):
        '''
        Given the instance type, start the multinode cluster.
        '''
        if self.instance_type == "aws":
            self.start_ec2_cluster()
        elif self.instance_type == "baremetal":
            self.start_baremetal_cluster()
        elif self.instance_type == "static_aws":
            self.start_static_aws_cluster()
        else:
            raise Exception, "Invalid instance type: %s" % self.instance_type

    def destroy_cluster(self):
        '''
        Allows the user to destroy existing cluster, if the instance type is
        'aws'. For 'baremetal' is yet to be supported.
        '''
        if self.instance_type == "aws":
            self.destroy_ec2_cluster()
        elif self.instance_type == "static_aws":
            self.destroy_static_aws_cluster()
        elif self.instance_type == "baremetal":
            self.log.info("not supported")
        else:
            raise Exception, "Invalid instance type: %s" % self.instance_type

    def start_baremetal_cluster(self):
        '''
        In case the user chose to have a local cluster, then starts it. If
        cluster already running, then have the latest version of FDS installed.
        '''
        baremetal = config.START_BAREMETAL % (self.baremetal_inventory,
                                              self.build_type)
        script = os.path.join(config.ANSIBLE_ROOT, baremetal)
        try:
            print script
            subprocess.call(["./%s" % script], shell=True)
        except Exception, e:
            self.log.exception(e)

    def destroy_baremetal_cluster(self):
        '''
        Not supported yet.
        '''
        pass

    def start_ec2_cluster(self):
        ec2 = config.START_EC2 % (self.cluster_name, self.instance_count,
                                  self.build_type)
        script = os.path.join(config.ANSIBLE_ROOT, ec2)
        try:
            print script
            subprocess.call(["./%s" % script], shell=True)
        except Exception, e:
            self.log.exception(e)

    def start_static_aws_cluster(self):
        static_ec2 = config.START_STATIC_AWS % (self.static_aws_inventory,
                                                self.build_type)
        script = os.path.join(config.ANSIBLE_ROOT, static_ec2)
        try:
            print script
            subprocess.call(["./%s" % script], shell=True)
        except Exception, e:
            self.log.exception(e)

    def destroy_static_aws_cluster(self):
        self.cluster_name = self.static_aws_inventory
        self.destroy_ec2_cluster()

    def destroy_ec2_cluster(self):
        '''
        If user created a FDS cluster using aws, then it can be destroyed by
        using this method
        '''
        ec2 = config.DESTROY_EC2 % (self.cluster_name)
        script = os.path.join(config.ANSIBLE_ROOT, ec2)
        try:
            subprocess.call(["./%s" % script], shell=True)
        except Exception, e:
            self.log.exception(e)
