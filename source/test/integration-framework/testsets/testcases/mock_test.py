#!/usr/bin/python
# Copyright 2015 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com

class MockTest(testcase.FDSTestCase):
    
    def __init__(self):
        pass
    
    def runTest(self):
        self.test_passed = False
        assert self.test_passed == True