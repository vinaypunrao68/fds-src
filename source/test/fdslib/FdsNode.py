#!/usr/bin/env python

# Copyright 2014 by Formation Data Systems, Inc.
#
import subprocess
import FdsSetup as inst
import BringUpCfg as fdscfg
import optparse, sys, time

class FdsNode:
    def __init__(self, node_cfg):
        self.node_cfg = node_cfg
        self.services = {}

    def add_service(self, service_id, service):
        if self.services.has_key(service_id):
            raise Exception("Service {} already exists".format(service_id))
        self.services[service_id] = service

    def get_service(self, service_id):
        return self.services[service_id]

    def remove_service(self, service_id):
        if self.services.has_key(service_id) is False:
            raise Exception("Service {} doesn't exists".format(service_id))
        del self.services[service_id]

    def get_fds_root(self):
        return self.node_cfg.nd_conf_dict['fds_root']

    def get_ip(self):
        return self.node_cfg.nd_conf_dict['ip']
