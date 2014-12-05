#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com


class TestSet(object):
    '''
    In our definition, a TestSet object is a set of test cases for a particular
    feature.
    '''
    def __init__(self, name, test_cases=[]):
        self.name = name
        self.test_cases = test_cases
