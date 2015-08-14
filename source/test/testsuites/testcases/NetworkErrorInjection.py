#!/usr/bin/python
#
# Copyright 2015 by Formation Data Systems, Inc.
#

"""
Module to wrap the usage of smchk for verifying sm tokens and metadata within system tests
"""

import TestCase
import sys
import os
import functools

testlib_path = os.path.join(os.getcwd(), '..',)
sys.path.append(testlib_path)

from testlib.utils.network_err_inj import NetworkErrorInjector
from fdslib.TestUtils import findNodeFromInv


class TestVerifyFailoverTimeout(TestCase.FDSTestCase):
    """
    Run a test that prevents network traffic to a service long enough that it should cause failover of
    that service.
    """
    def __init__(self, parameters=None, node=None, service=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_ServiceNetworkFailure,
                                             "Cause service to be failed due to network 'disconnect'")

        self.parameters = parameters
        self.node = node
        self.service = service

    def test_ServiceNetworkFailure(self):
        """
        Verify all nodes have the same tokens and that those tokens are correct
        """

        # Maps from the base port to the service port
        node_port_map = {
            'pm': lambda x: x,
            'sm': lambda x: str(int(x) + 1),
            'dm': lambda x: str(int(x) + 2),
            'am': lambda x: str(int(x) + 3),
            'o/fqm': lambda x: str(int(x) + 4)
        }

        # Resolve node/service into an IP/port
        # Get the FdsConfigRun object for this test.
        fdscfg = self.parameters["fdscfg"]
        nodes = fdscfg.rt_obj.cfg_nodes

        self.node = findNodeFromInv(nodes, self.node)

        nei = NetworkErrorInjector()
        target = (self.node.nd_conf_dict['ip'], node_port_map[self.service](self.node.nd_conf_dict['fds_port']))
        nei.add_drop_rules([target])

        # Keep the NetworkErrorInjector object so we can clean the errors out in a different test case
        self.parameters['nd_net_err_inj'] = nei

        return True



class UnsetAllNetworkErrors(TestCase.FDSTestCase):
    """
    Disable all previously set network errors
    """
    def __init__(self, parameters=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_UnsetAllTimeouts,
                                             "Cause service to be failed due to network 'disconnect'")
        self.parameters = parameters

    def test_UnsetAllTimeouts(self):

        if 'nd_net_err_inj' not in self.parameters:
            self.log.error("No network errors found in parameters list. Please verify that you have injected"
                           " errors before trying to remove them!")
            return False

        self.parameters['nd_net_err_inj'].clear_all_rules()

        return True