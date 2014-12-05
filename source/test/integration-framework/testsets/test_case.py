#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com


class TestCase(object):
    '''
    A particular System test is defined as a TestCase. Here, we define the
    common attributes that a particular test case must have. Therefore, all
    test cases inherit from TestCase
    '''
    def __init__(self, name):
        self.name = name
