#!/usr/bin/python
# Copyright 2014 by Formation Data Systems, Inc.
# Written by Philippe Ribeiro
# philippe@formationds.com


class TestSet(object):
    '''
    In our definition, a TestSet object is a set of test cases for a particular
    feature.
    
    Attributes:
    -----------
    name : str
        the name of the test set being executed
    test_cases: list
        a list of all the test cases for this test set.
    '''
    def __init__(self, name, test_cases=[]):
        self.name = name
        self.test_cases = test_cases

    def do_run(self):
        pass
    
    def do_work(self):
        '''
        Execute the test cases in a multithreaded fashion
        '''
