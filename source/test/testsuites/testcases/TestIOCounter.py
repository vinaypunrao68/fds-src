#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#

'''
Testcases to test FDS tiering functionality
'''

# FDS test-case pattern requirements.
import TestCase
import sys
import os
import pprint

sys.path.append(os.path.join(os.getcwd(), 'fdslib'))

from fdslib import SvcHandle


# This class wraps TrafficGen to provide test suite feedback
class VerifyAggregateIOCounters(TestCase.FDSTestCase):
    def __init__(self, parameters=None, counter=None, expected_value=None):
        super(self.__class__, self).__init__(parameters,
                                             self.__class__.__name__,
                                             self.test_IOCounter,
                                             "Test aggregate IO Counters")

        self.counter = counter
        self.expected_value = expected_value

    def test_IOCounter(self):
        """
        Test Case:
        Test the hybrid tiering features of FDS
        """

        # Verify that we received some counter to check.
        if self.counter is None or self.expected_value is None:
            self.log.error("No counter or expected value parameter found. Nothing will be tested. Failing Test!")
            return False

        fdscfg = self.parameters["fdscfg"]

        nodes = fdscfg.rt_obj.cfg_nodes

        aggregate_cntrs = 0

        prev_ip = ""
        for node in nodes:
            ip = node.nd_conf_dict['ip']
            # Prevent us from over-collecting if we're running on a single node
            if ip == prev_ip:
                continue
            prev_ip = ip

            port = node.nd_conf_dict['fds_port']

            # Try to capture counters
            try:
                svc_map = SvcHandle.SvcMap(ip, port)
            except Exception as e:
                self.log.error(e.message)
                self.log.error("Failed to get svc map. Unable to get counters. Failing.")
                return False

            svclist = svc_map.list()
            for entry in svclist:
                svc_id = entry[0]

                all_cntrs = svc_map.client(svc_id).getCounters('*')
                cntrs = all_cntrs.get(self.counter, None)

                if cntrs != None:
                    aggregate_cntrs += cntrs
                    continue

        print "CNTRS[{}] = {} EXPECTED = {}".format(self.counter, aggregate_cntrs, self.expected_value)
        # If it is not the expected value, fail the test
        if aggregate_cntrs != int(self.expected_value):
            return False

        return True