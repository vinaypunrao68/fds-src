#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import testsets.testcase as testcase

class MockTest(testcase.FDSTestCase):
    
    def __init__(self, parameters, config_file = None):
        super(MockTest, self).__init__(parameters=parameters,
                                       config_file=config_file)
        self.params = parameters
    
    def runTest(self):
        self.test_passed = False
        self.reportTestCaseResult(self.test_passed)