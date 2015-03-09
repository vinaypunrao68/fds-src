#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

# FDS test-case pattern requirements.
import unittest
import traceback

import xmlrunner
import TestCase


# Module-specific requirements
import sys
import time
import logging
import os
import shlex
import pdb
import random
import subprocess
import re

agents={
    "am" : "bare_am",
    "xdi" : "com\.formationds\.am\.Main",
    "om" : "com\.formationds\.om\.Main",
    "sm" : "StorMgr",
    "dm" : "DataMgr",
    "pm" : "platformd"
}

def get_pid_table():
    pid_table = {}
    for a in agents:
         cmd = "ps aux | egrep %s | grep -v egrep " % agents[a]
         try:
            output = subprocess.check_output(cmd, shell=True)
         except subprocess.CalledProcessError:
            continue
         for l in output.split("\n"):
             tokens = l.split()
             if len(tokens) > 2:
                 pid = int(tokens[1])
                 matches = [re.match("--fds-root=\/fds\/(\w+)", x) for x in tokens]
                 matches = [m for m in matches if m]
                 assert len(matches) == 1
                 node = matches[0].group(1)
                 if not a in pid_table:
                     pid_table[a] = [(pid, node)]
                 else:
                     pid_table[a].append((pid, node))
    return pid_table

class TestKillRandom(TestCase.FDSTestCase):
    def __init__(self, parameters=None, agent=None, node=None, random_agent=None, random_node=None):
        """
        When run by a qaautotest module test runner,
        "parameters" will have been populated with
        .ini configuration.
        """
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_KillRandom,
                                             "Kill a service as soon as comes up")
        if random_agent == "false":
            self.agent = agent
        else:
            self.agent = random.choice(agents.keys())

        if random_node == "false":
            self.node = node
        else:
            self.node = None


    def _get_pid(self, table, agent, node):
        if agent in table and len(table[agent]) > 0:
            p, n = random.choice(table[agent]) 
            if n == node or node is None:
                return p
        return -1
         
    def test_KillRandom(self):
        """
        Test Case:
        Kill a random service
        """
        #fdscfg = self.parameters["fdscfg"]
        #nodes = fdscfg.rt_obj.cfg_nodes
        #node =  random.choice(nodes) 
        pid = os.fork()
        if pid == 0:
            self.log.info("agent: %s" % (self.agent))
            pid_table = get_pid_table()
            pid = self._get_pid(pid_table, self.agent, self.node)
            # check = check_conditions(pid_table, self.agent, self.node)
            while (pid < 0):
                time.sleep(1)
                pid_table = get_pid_table()
                pid = self._get_pid(pid_table, self.agent, self.node)
            assert len(pid_table[self.agent]) > 0, "No process to kill"
            pid, node = random.choice(pid_table[self.agent])
            assert node == self.node or self.node is None, "Wrong node"
            self.log.info("Killing %s %d on %s" % (self.agent, pid, node))
            subprocess.call('kill -9  %s' % pid, shell=True)
            os._exit(0)
        return True

