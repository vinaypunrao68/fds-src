#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import testsets.testcase as testcase

class MockTest(testcase.FDSTestCase):
    
    def __init__(self, parameters=None, config_file=None, om_ip_address=None):
        super(MockTest, self).__init__(parameters=parameters,
                                       config_file=config_file,
                                       om_ip_address=om_ip_address)
        self.params = parameters
    
    def runTest(self):
        self.log.info("IP Address: %s" % self.om_ip_address)
        self.test_passed = True
        self.reportTestCaseResult(self.test_passed)