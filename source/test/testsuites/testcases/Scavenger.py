#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

"""
Module to wrap the usage of smchk for verifying sm tokens and metadata within system tests
"""

import TestCase
import sys
import os
import time
import re

testlib_path = os.path.join(os.getcwd(), '..',)
sys.path.append(testlib_path)
from testlib.sm.dlt import DLT as dlt

class TestRunScavenger(TestCase.FDSTestCase):
    """
    Run a test that verifies target dlt tokens
    """
    def __init__(self, parameters=None, nodes=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_RunScavenger,
                                             "run scavenger on target (or all) nodes")

        self.parameters = parameters
        self.nodes = nodes

    def _get_latest_sm_log(self, node):
        """
        Grabs the latest SM log from fds_root/var/logs/ directory
        :param node: Node object to find the sm log for
        :return: Full path to the latest SM log file
        """
        fds_dir = node.nd_conf_dict['fds_root']
        # Get a list of all log files.
        log_files = os.listdir(fds_dir + "/var/logs")
        # Make the full path for each log
        log_files = map(lambda x: os.path.join(fds_dir, 'var', 'logs', x), log_files)
        # Find just the SM logs
        log_files = filter(lambda x: 'sm' in x, log_files)
        # Now get just the latest log by sorting on mtime and taking the last one
        latest_log = sorted(log_files, key=os.path.getmtime)[-1]

        return latest_log


    def test_RunScavenger(self):
        """
        Run the scavenger
        """
        fdscfg = self.parameters["fdscfg"]
        om_node = fdscfg.rt_om_node

        if self.nodes is None:
            dlt_file = os.path.join(om_node.nd_conf_dict['fds_root'], 'var', 'logs', 'currentDLT')
            current_dlt = dlt.transpose_dlt(dlt.load_dlt(dlt_file))
            self.nodes = current_dlt.keys()

        for node_uuid in self.nodes:
            # Subtract 1 to get node UUID because we actually have SM UUIDs
            node_uuid -= 1

            call = ' '.join(['./fdsconsole.py', 'scavenger', 'enable', str(node_uuid)])
            res = om_node.nd_agent.exec_wait(call, fds_tools=True)
            if res != 0:
                self.log.error("enable scavenger failed for node {}".format(node_uuid))
                return False

            call = ' '.join(['./fdsconsole.py', 'scavenger', 'start', str(node_uuid)])
            res = om_node.nd_agent.exec_wait(call, fds_tools=True)
            if res != 0:
                self.log.error("start scavenger failed for node {}".format(node_uuid))
                return False

        return True