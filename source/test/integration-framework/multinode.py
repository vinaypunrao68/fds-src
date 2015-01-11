#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import logging
import os
import shutil
import subprocess
import sys

import config

class Multinode(object):
    
    '''
    The class Multinode supports creating a Multinode FDS cluster. 
    '''
    instance_types = ('aws', 'baremetal')
    build_types = ("nightly", "local")
    baremetal_inventory = "baremetal"
    logging.basicConfig(level=logging.INFO)
    log = logging.getLogger(__name__)
    
    def __init__(self, name="", instance_count=1, type='aws', build="nightly",
                 inventory=None):
        assert instance_count >= 0
        self.instance_count = instance_count
        if type not in self.instance_types:
            raise ValueError, 'Invalid Instance type. The supported versions' \
                ' are %s' % self.instance_types
            sys.exit(2)
        self.instance_type = type
        self.cluster_name = name
        if build not in self.build_types:
            raise ValueError, 'Invalid build type. The supported veersions' \
                ' are %s' % self.build_types
            sys.exit(2)
        self.build_type = build
        if self.__check_preconditions():
            self.start_cluster()
        else:
            self.destroy_cluster()  
        
    def __check_preconditions(self):
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
            vault = os.path.join(config.ROOT, config.VAULT_PASS)
            if not os.path.exists(vault):
                raise OSError, "Vault Pass file must exist before cluster is" \
                    " run."
                return False
            return True
        elif self.instance_type == "baremetal":
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
        if self.instance_type == "aws":
            self.start_ec2_cluster()
        elif self.instance_type == "baremetal":
            self.start_baremetal_cluster()
        else:
            raise Exception, "Invalid instance type: %s" % self.instance_type
    
    def start_baremetal_cluster(self):
        baremetal = config.START_BAREMETAL % (self.baremetal_inventory,
                                              self.build_type)
        script = os.path.join(config.ANSIBLE_ROOT, baremetal)
        try:
            print script
            subprocess.call(["./%s" % script], shell=True)
        except Exception, e:
            self.log.exception(e)
        # ./deploy_fds.sh system-nodes nightly
    
    def destroy_baremetal_cluster(self):
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
    
    def destroy_cluster(self):
        if self.instance_type == "aws":
            self.destroy_ec2_cluster()
        elif self.instance_type == "baremetal":
            print "not supported"
        else:
            raise Exception, "Invalid instance type: %s" % self.instance_type

    def destroy_ec2_cluster(self):
        ec2 = config.DESTROY_EC2 % (self.cluster_name)
        script = os.path.join(config.ANSIBLE_ROOT, ec2)
        try:
            subprocess.call(["./%s" % script], shell=True)
        except Exception, e:
            print e
    

if __name__ == "__main__":
    multinode = Multinode(name="philippe-ec2", instance_count=4, type="baremetal")
    # multinode.destroy_cluster()