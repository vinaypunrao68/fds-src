#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com
import testsets.testcase as testcase

class MockTest(testcase.FDSTestCase):
    
    def __init__(self, parameters):
        super(MockTest, self).__init__(parameters)
        self.params = parameters
    
    def runTest(self):
        self.test_passed = False
        assert self.test_passed == True