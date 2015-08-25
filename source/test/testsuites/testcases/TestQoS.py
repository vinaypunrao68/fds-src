#!/usr/bin/python

#
# Copyright 2015 Formation Data Systems, Inc.
#

import unittest
import traceback
import TestCase
from fdslib.TestUtils import check_localhost


import sys
import os

class TestQosRateLimit(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_QosRateLimit,
                                             "QoS Rate Limiting")
    
    def test_QosRateLimit(self):
        fdscfg = self.parameters["fdscfg"]
        
        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            ip = n.nd_conf_dict['ip']
            self.log.info("Apply QoS Rate Limit test to node %s." % n.nd_conf_dict['node-name'])
            
            cur_dir = os.getcwd()
            if check_localhost(ip):
                os.chdir(fdscfg.rt_env.env_fdsSrc + '/Build/linux-x86_64.debug/bin')
                status, stdout = n.nd_agent \
                              .exec_wait('./iodriver --fds-root=%s --workload=RateLimit' % n.nd_conf_dict['fds_root'],
                                         return_stdin=True)
            else:
                os.chdir('/fds/bin')
                status, stdout = n.nd_agent \
                              .exec_wait('/fds/bin/iodriver --fds-root=%s --workload=RateLimit' % n.nd_conf_dict['fds_root'],
                                         return_stdin=True)
            
            if stdout is not None:
                self.log.info(stdout)

            os.chdir(cur_dir)

            if status != 0:
                self.log.error("QoS Rate Limit test failed on node %s with status %d." %(n.nd_conf_dict['node-name'],
                                                                                         status))
                return False

            return True

class TestQosAssured(TestCase.FDSTestCase):
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_QosAssured,
                                             "QoS Assured Rate")
    
    def test_QosAssured(self):
        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes
        for n in nodes:
            ip = n.nd_conf_dict['ip']
            self.log.info("Apply QoS Assured rate test to node %s." % n.nd_conf_dict['node-name'])
            
            cur_dir = os.getcwd()
            if check_localhost(ip):
                os.chdir(fdscfg.rt_env.env_fdsSrc + '/Build/linux-x86_64.debug/bin')
                status, stdout = n.nd_agent \
                              .exec_wait('./iodriver --fds-root=%s --workload=Assured' % n.nd_conf_dict['fds_root'],
                                         return_stdin=True)
            else:
                os.chdir('/fds/bin')
                status, stdout = n.nd_agent \
                              .exec_wait('/fds/bin/iodriver --fds-root=%s --workload=Assured' % n.nd_conf_dict['fds_root'],
                                         return_stdin=True)

            if stdout is not None:
                self.log.info(stdout)
            
            os.chdir(cur_dir)
            
            if status != 0:
                self.log.error("QoS Assured Rate test failed on node %s with status %d." %(n.nd_conf_dict['node-name'],
                                                                                           status))
                return False
            
            return True

if __name__ == '__main__':
    TestCase.FDSTestCase.fdsGetCmdLineConfigs(sys.argv)
    unittest.main()
